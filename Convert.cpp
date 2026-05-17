#include "Convert.h"
#include <stack>
#include <set>
#include <fstream>
#include <iostream>
#include <climits>

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
// Assumes active/overflow flags and reset of reg are already set by caller.
bool Convert::tryColor(Data& data) {
    const int K = data.pool.getK();
    std::stack<int> S;
    int activeCount = 0;
    for (const auto& w : data.webs)
        if (w.active) activeCount++;

    // Phase 1: simplification — only push, never spill internally
    // If we get stuck it means the caller needs to spill more webs
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
// Algorithm: spilling, K
// Tries basic coloring first. If it fails, pre-spills up to K webs
// (highest degree first) one at a time until coloring succeeds.
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

        // tryColor left active flags in an inconsistent intermediate state.
        // Reset them before computing degrees so the degree calculation is correct.
        for (auto& w : data.webs)
            w.active = !w.overflow;

        // pick the highest-degree active web to spill next
        // rationale: removing the most-connected node frees the most edges
        int spillIdx = -1;
        int maxDeg   = -1;
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if (data.webs[i].overflow) continue; // already spilled
            int deg = Degree(data.webs[i], data.webs);
            if (deg > maxDeg) { maxDeg = deg; spillIdx = i; }
        }
        if (spillIdx == -1) break;
        data.webs[spillIdx].overflow = true;
    }

    // if we get here, even maxSpills wasn't enough
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
// Splits the web at splitIdx into two derived webs.
// Finds the best split point: the gap between two consecutive points where
// the fewest neighbors are alive on both sides.
// The split point is included in BOTH halves: w1 ends with isKill=true there,
// w2 starts with isDef=true there (models a store then a reload).
static void splitWeb(Data& data, int splitIdx, int& nextId) {
    // copy the web before erasing — erase invalidates the reference
    Web w = data.webs[splitIdx];
    auto& pts = w.allPoints;

    if ((int)pts.size() < 2) return;

    // find best split point: gap between pts[i] and pts[i+1] where the
    // fewest neighbors are alive on both sides of the cut
    int bestSplit = (int)pts.size() / 2;
    int minCross  = INT_MAX;

    for (int i = 0; i < (int)pts.size() - 1; i++) {
        int lineLeft  = pts[i].line;
        int lineRight = pts[i + 1].line;
        int cross = 0;
        for (const auto& other : data.webs) {
            if (other.id == w.id) continue;
            bool aliveLeft  = false;
            bool aliveRight = false;
            for (const auto& p : other.allPoints) {
                // alive on left: has a point at or before lineLeft that is not a pure kill
                if (p.line <= lineLeft  && !(p.isKill && !p.isDef)) aliveLeft  = true;
                // alive on right: has a point at or after lineRight that is not a pure def
                if (p.line >= lineRight && !(p.isDef  && !p.isKill)) aliveRight = true;
            }
            if (aliveLeft && aliveRight) cross++;
        }
        if (cross < minCross) {
            minCross  = cross;
            bestSplit = i;
        }
    }

    // build w1: pts[0..bestSplit], mark last point as kill
    Web w1(nextId++, w.Name + "_sp1");
    for (int i = 0; i <= bestSplit; i++) {
        Point p = pts[i];
        if (i == bestSplit) p.isKill = true;
        w1.allPoints.push_back(p);
    }

    // build w2: pts[bestSplit..end], mark first point as def
    Web w2(nextId++, w.Name + "_sp2");
    for (int i = bestSplit; i < (int)pts.size(); i++) {
        Point p = pts[i];
        if (i == bestSplit) p.isDef = true;
        w2.allPoints.push_back(p);
    }

    // remove original web first, then add the two derived ones
    data.webs.erase(data.webs.begin() + splitIdx);
    data.webs.push_back(w1);
    data.webs.push_back(w2);
}

// ── allocateSplitting ─────────────────────────────────────────────────────
// Algorithm: splitting, K
// Tries basic coloring first. If stuck, splits the highest-degree web at
// the point that removes the most interference edges, then retries.
// Repeats up to K times. If still infeasible, all webs go to memory.
void Convert::allocateSplitting(Data& data) {
    const int maxSplits = data.algorithmPar;
    const int K = data.pool.getK();

    // nextId must be beyond all existing web ids to avoid collisions
    int nextId = 0;
    for (const auto& w : data.webs)
        if (w.id >= nextId) nextId = w.id + 1;

    for (int splitBudget = 0; splitBudget <= maxSplits; splitBudget++) {
        // reset flags
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

        // tryColor left active flags inconsistent — reset before degree calc
        for (auto& w : data.webs)
            w.active = true;

        // pick highest-degree web to split (must have at least 2 points)
        int splitIdx = -1;
        int maxDeg   = -1;
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if ((int)data.webs[i].allPoints.size() < 2) continue;
            int deg = Degree(data.webs[i], data.webs);
            if (deg > maxDeg) { maxDeg = deg; splitIdx = i; }
        }

        if (splitIdx == -1) break;

        std::cerr << "Splitting web " << data.webs[splitIdx].Name
                  << " (id=" << data.webs[splitIdx].id
                  << ", deg=" << maxDeg
                  << ", points=" << data.webs[splitIdx].allPoints.size()
                  << ")" << std::endl;

        splitWeb(data, splitIdx, nextId);
    }

    // failed even after maxSplits — all to memory
    std::cerr << "Warning: splitting up to " << maxSplits
              << " web(s) was not enough to allocate with K=" << K
              << ". All webs spilled to memory." << std::endl;
    for (auto& w : data.webs) {
        w.overflow = true;
        w.reg = -1;
        w.active = false;
    }
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

    // count registers actually used
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
        // print register assignments (a register can map to multiple webs)
        for (int r : usedRegs) {
            for (int i = 0; i < (int)data.webs.size(); i++) {
                if (!data.webs[i].overflow && data.webs[i].reg == r)
                    *out << "r" << r << ": web" << i << "\n";
            }
        }
        // print spilled webs
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if (data.webs[i].overflow)
                *out << "M: web" << i << "\n";
        }
    }
}

void Convert::printResults(const Data& data) {
    // print to console in the same format
    writeOutput(data, "");
}