#ifndef DAPROJECT2_WEB_H
#define DAPROJECT2_WEB_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "LiveRange.h"

/**
 * @brief A web groups all live ranges of a variable that share at least one program point.
 *
 * A web is the fundamental unit for register allocation: one register (or memory slot)
 * is assigned per web. Webs are the nodes of the interference graph.
 */
struct Web {
    int id;                        ///< Unique identifier (used as graph vertex key).
    std::string Name;              ///< Original variable name.
    std::vector<LiveRange> ranges; ///< Individual live ranges merged into this web.
    std::vector<Point> allPoints;  ///< Sorted union of all points across all ranges.
    int  reg;       ///< Assigned register index (0-based), or -1 if unassigned.
    bool overflow;  ///< True if this web is spilled to memory.
    bool active;    ///< Used during graph-colouring simplification to mark removed nodes.

    /// @brief Constructs an empty web. @complexity O(1)
    Web(int id, const std::string& name): id(id), Name(name), reg(-1), overflow(false), active(true) {}

    /**
     * @brief Adds a live range and merges its points into allPoints.
     * @complexity O(p log p) where p is the total number of points.
     */
    void addRange(const LiveRange& lr) {
        ranges.push_back(lr);
        for (const auto& p : lr.points)
            mergePoint(allPoints, p);
    }

    /// @brief Sorts ranges by start line. @complexity O(r log r) where r = number of ranges.
    void sortRange() {
        std::sort(ranges.begin(), ranges.end(), LiveRange::comp);
    }

    /// @brief Rebuilds allPoints from scratch after range modifications. @complexity O(p^2)
    void rebuildPoints() {
        allPoints.clear();
        for (const auto& lr : ranges)
            for (const auto& p : lr.points)
                mergePoint(allPoints, p);
    }

    /**
     * @brief Returns true if this web interferes with another.
     *
     * Two webs interfere if they share at least one program point where both
     * are simultaneously live. A def/kill boundary at the same line does NOT
     * constitute interference.
     * @complexity O(p1 + p2) where p1, p2 are the point counts of each web.
     */
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

    /**
     * @brief Formats allPoints as a comma-separated string (e.g. "1+,2,3,4-").
     * @complexity O(p)
     */
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
        /**
         * @brief Inserts a point into a sorted vector, absorbing duplicates.
         * @complexity O(log p) for the binary search + O(p) for insertion in the worst case.
         */
        static void mergePoint(std::vector<Point>& vec, const Point& p) {
            auto it = std::lower_bound(vec.begin(), vec.end(), p);
            if (it != vec.end() && it->line == p.line)
                it->absorb(p);
            else
                vec.insert(it, p);
        }
};

#endif // DAPROJECT2_WEB_H
