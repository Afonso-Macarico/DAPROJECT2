#ifndef DAPROJECT2_POINT_H
#define DAPROJECT2_POINT_H

#include <string>
#include <vector>
#include <algorithm>

/**
 * @brief Represents a single program point (line number) within a live range.
 *
 * A point may be a definition (isDef), a last-use/kill (isKill), or a plain
 * intermediate live point. A point can simultaneously be a def and a kill
 * (e.g. i = i + 1).
 */
struct Point {
    int  line;    ///< Program line number.
    bool isDef;   ///< True if the variable is defined at this point.
    bool isKill;  ///< True if this is the last use of the variable's value.

    /// @brief Constructs a Point. @complexity O(1)
    Point(int l, bool d, bool k) : line(l), isDef(d), isKill(k) {}

    /**
     * @brief Merges flags from another point at the same line number.
     * @param o Point to absorb (must share the same line).
     * @complexity O(1)
     */
    void absorb(const Point& o) {
        isDef  = isDef  || o.isDef;
        isKill = isKill || o.isKill;
    }

    /// @brief Compares by line number. @complexity O(1)
    bool operator<(const Point& o) const { return line < o.line; }
};

#endif //DAPROJECT2_POINT_H
