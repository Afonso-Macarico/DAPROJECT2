#ifndef DAPROJECT2_WEB_H
#define DAPROJECT2_WEB_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "LiveRange.h"

struct Web {
    int id;
    std::string Name;
    std::vector<LiveRange> ranges;
    std::vector<Point> allPoints;
    int  reg;       ///< assigned register (0-based), or -1 if unassigned
    bool overflow;  ///< true -> spilled to memory
    bool active;    ///< used during graph-colouring simplification
    Web(int id, const std::string& name): id(id), Name(name), reg(-1), overflow(false), active(true) {}
    void addRange(const LiveRange& lr) {
        ranges.push_back(lr);
        for (const auto& p : lr.points)
            mergePoint(allPoints, p);
    }
    void sortRange() {
        std::sort(ranges.begin(), ranges.end(), LiveRange::comp);
    }
    void rebuildPoints() {
        allPoints.clear();
        for (const auto& lr : ranges)
            for (const auto& p : lr.points)
                mergePoint(allPoints, p);
    }
    bool OverlapCheck(const Web& other) const {
        int i = 0, j = 0;
        while (i < (int)allPoints.size() && j < (int)other.allPoints.size()) {
            if      (allPoints[i].line < other.allPoints[j].line) { ++i; continue; }
            else if (other.allPoints[j].line < allPoints[i].line) { ++j; continue; }
            const Point& p = allPoints[i];
            const Point& q = other.allPoints[j];
            if (!(p.isKill && q.isDef) && !(q.isKill && p.isDef))
                return true;
            ++i; ++j;
        }
        return false;
    }
    std::string pointsToString() const {
        std::ostringstream oss;
        bool first = true;
        for (const auto& p : allPoints) {
            if (!first) oss << ',';
            first = false;
            oss << p.line;
            if (p.isDef)  oss << '+';
            if (p.isKill) oss << '-';
        }
        return oss.str();
    }
    private:
        static void mergePoint(std::vector<Point>& vec, const Point& p) {
            auto it = std::lower_bound(vec.begin(), vec.end(), p);
            if (it != vec.end() && it->line == p.line)
                it->absorb(p);
            else
                vec.insert(it, p);
        }
};

#endif // DAPROJECT2_WEB_H