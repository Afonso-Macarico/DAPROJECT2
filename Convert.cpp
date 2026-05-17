#include "Convert.h"
#include <stack>
#include <set>
#include <fstream>
#include <iostream>
#include <climits>
#include <cmath>

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
    if (data.algorithm == "spilling") {
        allocateSpilling(data);
        return;
    }
    if (data.algorithm == "splitting") {
        allocateSplitting(data);
        return;
    }
    if (data.algorithm == "free") {
        allocateFree(data);
        return;
    }
    // default: basic
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

        // if no node had degree < K, allocation is infeasible — basic does not spill
        if (!pushed) {
            std::cerr << "Warning: register allocation with K=" << K
                      << " is not possible." << std::endl;
            for (auto& w : data.webs) {
                w.reg = -1;
                w.overflow = true;
                w.active = false;
            }
            return;
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
}

// ── tryColor ──────────────────────────────────────────────────────────────
// Runs the greedy coloring phase only (no spilling decisions).
// Returns true if ALL non-overflow webs got a colour, false otherwise.
bool Convert::tryColor(Data& data) {
    const int K = data.pool.getK();
    std::stack<int> S;
    int activeCount = 0;

    // Clear old state tracking and safely isolate register fields from previous failed passes
    for (auto& w : data.webs) {
        w.reg = -1;
        w.active = !w.overflow;
        if (w.active) activeCount++;
    }

    // Phase 1: simplification — only push, never spill internally
    while (activeCount > 0) {
        bool pushed = false;
        for (auto& web : data.webs) {
            if (!web.active) continue;
            if (Degree(web, data.webs) < K) {
                S.push(web.id);
                web.active = false;
                activeCount--;
                pushed = true;
            }
        }
        if (!pushed) {
            // stuck — can't simplify without spilling, signal failure
            return false;
        }
    }

    // Phase 2: coloring
    while (!S.empty()) {
        int id = S.top(); S.pop();
        int idx = findIdx(id, data.webs);
        if (idx == -1) continue;

        Web& web = data.webs[idx];
        web.active = true;

        std::set<int> usedColours;
        for (const auto& other : data.webs) {
            if (!other.active || other.id == web.id) continue;
            if (!web.OverlapCheck(other)) continue;
            if (other.reg >= 0) usedColours.insert(other.reg);
        }

        for (int c = 0; c < K; c++) {
            if (usedColours.find(c) == usedColours.end()) {
                web.reg = c;
                break;
            }
        }
        if (web.reg == -1) return false;
    }
    return true;
}

// ── allocateSpilling ──────────────────────────────────────────────────────
void Convert::allocateSpilling(Data& data) {
    const int maxSpills = data.algorithmPar;
    const int K = data.pool.getK();

    for (int spillBudget = 0; spillBudget <= maxSpills; spillBudget++) {
        // reset everything
        for (auto& w : data.webs) {
            w.reg = -1;
            w.active = !w.overflow; // overflow=pre-spilled, stays out
        }

        if (tryColor(data)) {
            // success
            int spillCount = 0;
            for (const auto& w : data.webs) if (w.overflow) spillCount++;
            if (spillCount > 0)
                std::cerr << "Spilling: allocation succeeded after spilling "
                          << spillCount << " web(s)." << std::endl;
            return;
        }

        if (spillBudget == maxSpills) break;

        // Reset active flags explicitly before recalculating accurate neighbor degrees
        for (auto& w : data.webs)
            w.active = !w.overflow;

        // pick the highest-degree active web to spill next
        // tiebreak: prefer the web with the most points (largest live range)
        int spillIdx  = -1;
        int maxDeg    = -1;
        int maxPoints = -1;
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if (data.webs[i].overflow) continue; // already spilled
            int deg = Degree(data.webs[i], data.webs);
            int pts = (int)data.webs[i].allPoints.size();
            if (deg > maxDeg || (deg == maxDeg && pts > maxPoints)) {
                maxDeg = deg; maxPoints = pts; spillIdx = i;
            }
        }
        if (spillIdx == -1) break;
        data.webs[spillIdx].overflow = true;
    }

    std::cerr << "Warning: spilling up to " << maxSpills
              << " web(s) was not enough to allocate with K=" << K
              << ". All webs spilled to memory." << std::endl;
    for (auto& w : data.webs) {
        w.overflow = true;
        w.reg = -1;
        w.active = false;
    }
}

// ── splitWeb ──────────────────────────────────────────────────────────────
static void splitWeb(Data& data, int splitIdx, int& nextId) {
    Web w = data.webs[splitIdx];
    auto& pts = w.allPoints;

    if ((int)pts.size() < 2) return;

    int bestSplit = 0;
    int minCross  = INT_MAX;

    // Evaluate gaps between index i and i+1
    for (int i = 0; i < (int)pts.size() - 1; i++) {
        int lineLeft  = pts[i].line;
        int lineRight = pts[i + 1].line;
        int cross = 0;

        for (const auto& other : data.webs) {
            if (other.id == w.id) continue;
            bool aliveLeft  = false;
            bool aliveRight = false;
            for (const auto& p : other.allPoints) {
                if (p.line <= lineLeft  && !(p.isKill && !p.isDef)) aliveLeft  = true;
                if (p.line >= lineRight && !(p.isDef  && !p.isKill)) aliveRight = true;
            }
            if (aliveLeft && aliveRight) cross++;
        }
        if (cross < minCross) {
            minCross  = cross;
            bestSplit = i;
        }
    }

    // build w1: points from index 0 up to bestSplit
    Web w1(nextId++, w.Name + "_sp1");
    for (int i = 0; i <= bestSplit; i++) {
        Point p = pts[i];
        if (i == bestSplit) p.isKill = true; // Ends strictly here
        w1.allPoints.push_back(p);
    }

    // build w2: points starting strictly from bestSplit + 1 to the end
    Web w2(nextId++, w.Name + "_sp2");
    for (int i = bestSplit + 1; i < (int)pts.size(); i++) {
        Point p = pts[i];
        if (i == bestSplit + 1) p.isDef = true; // Starts strictly here
        w2.allPoints.push_back(p);
    }

    // Modify array structure directly via assignment to ensure vector layout/sequence order metrics don't scramble
    data.webs[splitIdx] = w1;
    data.webs.push_back(w2);
}

// ── allocateSplitting ─────────────────────────────────────────────────────
void Convert::allocateSplitting(Data& data) {
    const int maxSplits = data.algorithmPar;
    const int K = data.pool.getK();

    int nextId = 0;
    for (const auto& w : data.webs)
        if (w.id >= nextId) nextId = w.id + 1;

    for (int splitBudget = 0; splitBudget <= maxSplits; splitBudget++) {
        // Clear variables flags before trial color pass
        for (auto& w : data.webs) {
            w.reg = -1;
            w.overflow = false;
            w.active = true;
        }

        if (tryColor(data)) {
            if (splitBudget > 0)
                std::cerr << "Splitting: allocation succeeded after "
                          << splitBudget << " split(s)." << std::endl;
            return;
        }

        if (splitBudget == maxSplits) break;

        // Reset tracking states globally before checking accurate degrees
        for (auto& w : data.webs)
            w.active = true;

        // pick highest-degree web to split (must have at least 2 points)
        // tiebreak: prefer the web with the most points (most splitting potential)
        int splitIdx  = -1;
        int maxDeg    = -1;
        int maxPoints = -1;
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if ((int)data.webs[i].allPoints.size() < 2) continue;
            int deg = Degree(data.webs[i], data.webs);
            int pts = (int)data.webs[i].allPoints.size();
            if (deg > maxDeg || (deg == maxDeg && pts > maxPoints)) {
                maxDeg = deg; maxPoints = pts; splitIdx = i;
            }
        }

        if (splitIdx == -1) break;

        std::cerr << "Splitting web " << data.webs[splitIdx].Name
                  << " (id=" << data.webs[splitIdx].id
                  << ", deg=" << maxDeg
                  << ", points=" << data.webs[splitIdx].allPoints.size()
                  << ")" << std::endl;

        splitWeb(data, splitIdx, nextId);
    }

    std::cerr << "Warning: splitting up to " << maxSplits
              << " web(s) was not enough to allocate with K=" << K
              << ". All webs spilled to memory." << std::endl;
    for (auto& w : data.webs) {
        w.overflow = true;
        w.reg = -1;
        w.active = false;
    }
}

void Convert::allocateFree(Data& data) {
    const int K = data.pool.getK();

    for (auto& w : data.webs) {
        w.reg      = -1;
        w.overflow = false;
        w.active   = true;
    }

    // flat list pairing each LiveRange with its parent Web
    struct RangeEntry {
        LiveRange* lr;
        Web*       web;
    };

    std::vector<RangeEntry> sorted;
    for (auto& w : data.webs)
        for (auto& lr : w.ranges)
            sorted.push_back({&lr, &w});

    std::sort(sorted.begin(), sorted.end(), [](const RangeEntry& a, const RangeEntry& b) {
        return a.lr->start < b.lr->start;
    });

    struct ActiveEntry {
        LiveRange* lr;
        Web*       web;
        int        reg;
    };

    std::vector<ActiveEntry> active;
    std::vector<bool> freeRegs(K, true);

    for (auto& entry : sorted) {
        LiveRange* lr  = entry.lr;
        Web*       web = entry.web;

        // expire ranges that ended before this one starts — gaps are genuinely free
        std::vector<ActiveEntry> stillActive;
        for (auto& a : active) {
            if (a.lr->end < lr->start) {
                freeRegs[a.reg] = true;
            } else {
                stillActive.push_back(a);
            }
        }
        active = stillActive;

        // find a free register
        int chosen = -1;
        for (int r = 0; r < K; r++) {
            if (freeRegs[r]) { chosen = r; break; }
        }

        if (chosen != -1) {
            freeRegs[chosen] = false;
            active.push_back({lr, web, chosen});

            if (web->reg == -1) {
                web->reg = chosen;
            } else if (web->reg != chosen) {
                // two ranges of same web got different registers — spill the web
                web->overflow = true;
            }

        } else {
            // no free register — find active range with furthest end point
            ActiveEntry* victim = nullptr;
            for (auto& a : active)
                if (!victim || a.lr->end > victim->lr->end)
                    victim = &a;

            if (victim && lr->end < victim->lr->end) {
                // spill the victim, give its register to current range
                int freed        = victim->reg;
                victim->web->reg      = -1;
                victim->web->overflow = true;
                active.erase(std::remove_if(active.begin(), active.end(),
                    [&](const ActiveEntry& a) { return a.lr == victim->lr; }),
                    active.end());

                freeRegs[freed] = false;
                active.push_back({lr, web, freed});

                if (web->reg == -1) {
                    web->reg = freed;
                } else if (web->reg != freed) {
                    web->overflow = true;
                }
            } else {
                // current range ends latest — spill its web directly
                web->overflow = true;
            }
        }
    }

    int spillCount = 0;
    for (const auto& w : data.webs) if (w.overflow) spillCount++;
    if (spillCount > 0)
        std::cerr << "Free (Linear Scan): allocation completed with "
                  << spillCount << " web(s) spilled to memory." << std::endl;
}
// helper: format a single web's ranges as "1+,2,3,4,5,6-" sorted ascending
static std::string formatWeb(const Web& w) {
    std::string out;
    bool first = true;
    for (const auto& p : w.allPoints) {
        if (!first) out += ',';
        first = false;
        out += std::to_string(p.line);
        if (p.isDef)  out += '+';
        if (p.isKill) out += '-';
    }
    return out;
}

void Convert::writeOutput(const Data& data, const std::string& outputFile) {
    std::ostream* out = &std::cout;
    std::ofstream file;
    if (!outputFile.empty()) {
        file.open(outputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open output file: " << outputFile << std::endl;
            return;
        }
        out = &file;
    }

    std::set<int> usedRegs;
    for (const auto& w : data.webs)
        if (!w.overflow && w.reg >= 0)
            usedRegs.insert(w.reg);

    bool allSpilled = usedRegs.empty();

    *out << "# Total number of webs followed by the listing of the program points of each one\n";
    *out << "# program points in each web are sorted in ascending order\n";
    *out << "webs: " << data.webs.size() << "\n";
    for (int i = 0; i < (int)data.webs.size(); i++) {
        *out << "web" << i << ": " << formatWeb(data.webs[i]) << "\n";
    }

    *out << "# Total number of registers used, followed by assignment to webs\n";
    *out << "registers: " << (allSpilled ? 0 : (int)usedRegs.size()) << "\n";

    if (allSpilled) {
        for (int i = 0; i < (int)data.webs.size(); i++)
            *out << "M: web" << i << "\n";
    } else {
        for (int r : usedRegs) {
            for (int i = 0; i < (int)data.webs.size(); i++) {
                if (!data.webs[i].overflow && data.webs[i].reg == r)
                    *out << "r" << r << ": web" << i << "\n";
            }
        }
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if (data.webs[i].overflow)
                *out << "M: web" << i << "\n";
        }
    }
}

void Convert::printResults(const Data& data) {
    writeOutput(data, "");
}