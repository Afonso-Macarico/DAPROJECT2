#include "Convert.h"
#include <stack>
#include <set>

Graph<int> Convert::BuildGraph(const Data& data) {
    Graph<int> g;

    // Add one vertex per web
    for (const auto& web : data.webs)
        g.addVertex(web.id);

    // Add an edge between any two webs that interfere
    for (int i = 0; i < (int)data.webs.size(); i++)
        for (int j = i + 1; j < (int)data.webs.size(); j++)
            if (data.webs[i].OverlapCheck(data.webs[j])) {
                g.addEdge(data.webs[i].id, data.webs[j].id, 1);
                g.addEdge(data.webs[j].id, data.webs[i].id, 1); // undirected
            }
    return g;
}

int Convert::Degree(const Web& web, const std::vector<Web>& webs) {
    int deg = 0;
    for (const auto& other : webs) {
        if (other.active && other.id != web.id && web.OverlapCheck(other)) deg ++;
    }
    return deg;
}

int Convert::findIdx(int id, const std::vector<Web>& webs) {
    for (int i = 0; i < (int)webs.size(); i++)
        if (webs[i].id == id) return i;
    return -1;
}

void Convert::allocate(Data& data) {
    const int K = data.pool.getK();

    // reset
    for (auto& w : data.webs) {
        w.reg = -1;
        w.overflow = false;
        w.active = true;
    }

    std::stack<int> S; // stores web ids in simplification order

    // ── Phase 1: Simplification ──
    int activeCount = (int)data.webs.size();

    while (activeCount > 0) {
        bool pushed = false;

        // push all active nodes with degree < K
        for (auto& web : data.webs) {
            if (!web.active) continue;
            if (Degree(web, data.webs) < K) {
                S.push(web.id);
                web.active = false;
                activeCount--;
                pushed = true;
            }
        }

        // if no node had degree < K, spill the highest-degree active node
        if (!pushed) {
            int spillIdx = -1;
            int maxDeg = -1;
            for (int i = 0; i < (int)data.webs.size(); i++) {
                if (!data.webs[i].active) continue;
                int deg = Degree(data.webs[i], data.webs);
                if (deg > maxDeg) {
                    maxDeg = deg;
                    spillIdx = i;
                }
            }
            if (spillIdx != -1) {
                data.webs[spillIdx].overflow = true;
                data.webs[spillIdx].active = false;
                activeCount--;
                // spilled nodes do NOT go on the stack — they get no colour
            }
        }
    }

    // ── Phase 2: Colouring ──
    while (!S.empty()) {
        int id = S.top(); S.pop();
        int idx = findIdx(id, data.webs);
        if (idx == -1) continue;

        Web& web = data.webs[idx];
        web.active = true; // restore to graph

        // collect colours already used by active (already coloured) neighbours
        std::set<int> usedColours;
        for (const auto& other : data.webs) {
            if (!other.active) continue;
            if (other.id == web.id) continue;
            if (!web.OverlapCheck(other)) continue;
            if (other.reg == -1) continue;
            usedColours.insert(other.reg);
        }

        // assign lowest available colour
        for (int c = 0; c < K; c++) {
            if (usedColours.find(c) == usedColours.end()) {
                web.reg = c;
                break;
            }
        }

        // if no colour found (shouldn't happen in basic, but guard anyway)
        if (web.reg == -1)
            web.overflow = true;
    }

    // report result to console
    int spillCount = 0;
    for (const auto& w : data.webs)
        if (w.overflow) spillCount++;

    if (spillCount > 0) {
        std::cerr << "Warning: register allocation infeasible with K=" << K
                  << ". " << spillCount << " web(s) spilled to memory." << std::endl;
    }
}