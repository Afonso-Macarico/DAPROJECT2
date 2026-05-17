#ifndef DAPROJECT2_LIVERANGE_H
#define DAPROJECT2_LIVERANGE_H
#include <string>
#include "Point.h"

struct LiveRange {
    int id;
    std::string Name;
    std::vector<Point> points;
    int start;
    int end;
    LiveRange(int id, const std::string& name, std::vector<Point> pts)
        : id(id), Name(name), points(std::move(pts)), start(0), end(0)
    {
        sortAndDedupe();
        recomputeBounds();
    }

    void sortAndDedupe() {
        std::sort(points.begin(), points.end());
        std::vector<Point> deduped;
        for (auto& p : points) {
            if (!deduped.empty() && deduped.back().line == p.line)
                deduped.back().absorb(p);
            else
                deduped.push_back(p);
        }
        points = std::move(deduped);
    }
    void recomputeBounds() {
        if (points.empty()) { start = end = 0; return; }
        start = points.front().line;
        end   = points.back().line;
    }

    bool containsLine(int l) const {
        for (const auto& p : points)
            if (p.line == l) return true;
        return false;
    }

    const Point* getPoint(int l) const {
        for (const auto& p : points)
            if (p.line == l) return &p;
        return nullptr;
    }

    bool overlaps(const LiveRange& other) const {
        for (const auto& p : points) {
            const Point* q = other.getPoint(p.line);
            if (!q) continue;
            if (p.isKill && q->isDef)  continue;
            if (q->isKill && p.isDef)  continue;
            return true;
        }
        return false;
    }

    static bool comp(const LiveRange& a, const LiveRange& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.end < b.end;
    }
};

#endif //DAPROJECT2_LIVERANGE_H