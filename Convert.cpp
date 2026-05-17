/**
 * @file Convert.cpp
 * @brief Implementation of graph construction and register allocation algorithms.
 */
#include "Convert.h"
#include <stack>
#include <set>
#include <fstream>
#include <iostream>
#include <climits>
#include <cmath>

Graph<int> Convert::BuildGraph(const Data& data) {
    Graph<int> g;

    for (const auto& web : data.webs)
        g.addVertex(web.id);

    for (int i = 0; i < (int)data.webs.size(); i++)
        for (int j = i + 1; j < (int)data.webs.size(); j++)
            if (data.webs[i].OverlapCheck(data.webs[j])) {
                g.addEdge(data.webs[i].id, data.webs[j].id, 1);
                g.addEdge(data.webs[j].id, data.webs[i].id, 1);
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
    const int K = data.pool.getK();

    for (auto& w : data.webs) {
        w.reg = -1;
        w.overflow = false;
        w.active = true;
    }

    std::stack<int> S;

    int activeCount = (int)data.webs.size();

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

    while (!S.empty()) {
        int id = S.top(); S.pop();
        int idx = findIdx(id, data.webs);
        if (idx == -1) continue;

        Web& web = data.webs[idx];
        web.active = true;

        std::set<int> usedColours;
        for (const auto& other : data.webs) {
            if (!other.active) continue;
            if (other.id == web.id) continue;
            if (!web.OverlapCheck(other)) continue;
            if (other.reg == -1) continue;
            usedColours.insert(other.reg);
        }

        for (int c = 0; c < K; c++) {
            if (usedColours.find(c) == usedColours.end()) {
                web.reg = c;
                break;
            }
        }

        if (web.reg == -1)
            web.overflow = true;
    }
}

bool Convert::tryColor(Data& data) {
    const int K = data.pool.getK();
    std::stack<int> S;
    int activeCount = 0;

    for (auto& w : data.webs) {
        w.reg = -1;
        w.active = !w.overflow;
        if (w.active) activeCount++;
    }

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
            return false;
        }
    }

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

void Convert::allocateSpilling(Data& data) {
    const int maxSpills = data.algorithmPar;
    const int K = data.pool.getK();

    for (int spillBudget = 0; spillBudget <= maxSpills; spillBudget++) {
        for (auto& w : data.webs) {
            w.reg = -1;
            w.active = !w.overflow;
        }

        if (tryColor(data)) {
            int spillCount = 0;
            for (const auto& w : data.webs) if (w.overflow) spillCount++;
            if (spillCount > 0)
                std::cerr << "Allocation succeeded after spilling " << spillCount << " web(s)." << std::endl;
            return;
        }

        if (spillBudget == maxSpills) break;

        for (auto& w : data.webs)
            w.active = !w.overflow;

        int spillIdx = -1;
        int maxDeg = -1;
        int maxPoints = -1;
        for (int i = 0; i < (int)data.webs.size(); i++) {
            if (data.webs[i].overflow) continue;
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

static void splitWeb(Data& data, int splitIdx, int& nextId) {
    Web w = data.webs[splitIdx];
    auto& pts = w.allPoints;

    if ((int)pts.size() < 2) return;

    int bestSplit = 0;
    int minCross = INT_MAX;

    for (int i = 0; i < (int)pts.size() - 1; i++) {
        int lineLeft = pts[i].line;
        int lineRight = pts[i + 1].line;
        int cross = 0;

        for (const auto& other : data.webs) {
            if (other.id == w.id) continue;
            bool aliveLeft = false;
            bool aliveRight = false;
            for (const auto& p : other.allPoints) {
                if (p.line <= lineLeft  && !(p.isKill && !p.isDef)) aliveLeft  = true;
                if (p.line >= lineRight && !(p.isDef  && !p.isKill)) aliveRight = true;
            }
            if (aliveLeft && aliveRight) cross++;
        }
        if (cross < minCross) {
            minCross = cross;
            bestSplit = i;
        }
    }

    Web w1(nextId++, w.Name + "_sp1");
    for (int i = 0; i <= bestSplit; i++) {
        Point p = pts[i];
        if (i == bestSplit) p.isKill = true;
        w1.allPoints.push_back(p);
    }

    Web w2(nextId++, w.Name + "_sp2");
    for (int i = bestSplit + 1; i < (int)pts.size(); i++) {
        Point p = pts[i];
        if (i == bestSplit + 1) p.isDef = true;
        w2.allPoints.push_back(p);
    }

    data.webs[splitIdx] = w1;
    data.webs.push_back(w2);
}

void Convert::allocateSplitting(Data& data) {
    const int maxSplits = data.algorithmPar;
    const int K = data.pool.getK();

    int nextId = 0;
    for (const auto& w : data.webs)
        if (w.id >= nextId) nextId = w.id + 1;

    for (int splitBudget = 0; splitBudget <= maxSplits; splitBudget++) {
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

        for (auto& w : data.webs)
            w.active = true;

        int splitIdx = -1;
        int maxDeg = -1;
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
    struct RangeEntry {
        LiveRange* lr;
        Web* web;
    };
    struct ActiveEntry {
        LiveRange* lr;
        Web* web;
        int reg;
    };


    const int K = data.pool.getK();

    for (auto& w : data.webs) {
        w.reg = -1;
        w.overflow = false;
        w.active = true;
    }


    std::vector<RangeEntry> sorted;
    for (auto& w : data.webs)
        for (auto& lr : w.ranges)
            sorted.push_back({&lr, &w});

    std::sort(sorted.begin(), sorted.end(), [](const RangeEntry& a, const RangeEntry& b) {
        return a.lr->start < b.lr->start;
    });

    
    std::vector<ActiveEntry> active;
    std::vector<bool> freeRegs(K, true);

    for (auto& entry : sorted) {
        LiveRange* lr = entry.lr;
        Web* web = entry.web;

        std::vector<ActiveEntry> stillActive;
        for (auto& a : active) {
            if (a.lr->end < lr->start) {
                freeRegs[a.reg] = true;
            } else {
                stillActive.push_back(a);
            }
        }
        active = stillActive;

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
                web->overflow = true;
            }

        } else {
            ActiveEntry* victim = nullptr;
            for (auto& a : active)
                if (!victim || a.lr->end > victim->lr->end)
                    victim = &a;

            if (victim && lr->end < victim->lr->end) {
                int freed = victim->reg;
                victim->web->reg = -1;
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
                web->overflow = true;
            }
        }
    }

    int spillCount = 0;
    for (const auto& w : data.webs) if (w.overflow) spillCount++;
    if (spillCount > 0)
        std::cerr << "Linear scan: " << spillCount << " web(s) spilled to memory." << std::endl;
}
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