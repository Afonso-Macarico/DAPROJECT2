#ifndef DAPROJECT2_LIVERANGE_H
#define DAPROJECT2_LIVERANGE_H
#include <string>
#include "Point.h"

/**
 * @brief A single contiguous live range for a variable.
 *
 * Stores the sorted list of Points from the first definition to the last use.
 * Multiple LiveRanges for the same variable are merged into a Web.
 */
struct LiveRange {
    int id;                    ///< Unique identifier.
    std::string Name;          ///< Variable name this range belongs to.
    std::vector<Point> points; ///< Sorted list of program points in this range.
    int start;                 ///< First line number of the range.
    int end;                   ///< Last line number of the range.

    /**
     * @brief Constructs a LiveRange and sorts/deduplicates its points.
     * @complexity O(p log p) where p is the number of points.
     */
    LiveRange(int id, const std::string& name, std::vector<Point> pts)
        : id(id), Name(name), points(std::move(pts)), start(0), end(0)
    {
        sortAndDedupe();
        recomputeBounds();
    }

    /**
     * @brief Sorts points by line and merges duplicates via absorb().
     * @complexity O(p log p)
     */
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

    /// @brief Updates start and end from the current points list. @complexity O(1)
    void recomputeBounds() {
        if (points.empty()) { start = end = 0; return; }
        start = points.front().line;
        end   = points.back().line;
    }

    /// @brief Returns true if line l is in this range. @complexity O(p)
    bool containsLine(int l) const {
        for (const auto& p : points)
            if (p.line == l) return true;
        return false;
    }

    /// @brief Returns a pointer to the Point at line l, or nullptr. @complexity O(p)
    const Point* getPoint(int l) const {
        for (const auto& p : points)
            if (p.line == l) return &p;
        return nullptr;
    }

    /**
     * @brief Returns true if this range overlaps with another.
     *
     * Two ranges do NOT overlap at a shared line if one ends (isKill) and
     * the other starts (isDef) there — the register is free between them.
     * @complexity O(p)
     */
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

    /// @brief Comparator: orders ranges by start, then by end. @complexity O(1)
    static bool comp(const LiveRange& a, const LiveRange& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.end < b.end;
    }
};

#endif //DAPROJECT2_LIVERANGE_H
