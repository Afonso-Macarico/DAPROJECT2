/**
* @file LiveRange.h
 * @brief Defines the LiveRange data structure.
 */

#ifndef DAPROJECT2_LIVERANGE_H
#define DAPROJECT2_LIVERANGE_H
#include <string>
#include <vector>
#include <algorithm>
#include <set>

struct LiveRange {
    int id;
    std::string Name;
    int start;
    int end;
    std::vector<int> points;

    LiveRange(int id, std::string name, int start, int end, std::vector<int> pts)
        : id(id), Name(name), start(start), end(end), points(pts) {}

    bool overlaps(const LiveRange& other) const {
        // Two ranges interfere if they share any program point,
        // EXCEPT when one ends via a use (-) on the same line the other starts via a def (+)
        // In that case they do NOT interfere (the spec subtlety in section 2.5)
        for (int p : points) {
            for (int q : other.points) {
                if (p == q) {
                    // check the edge case: one ends here (last point, is a use)
                    // and the other starts here (first point, is a def) — no interference
                    bool thisEndsHere  = (p == end);
                    bool otherStartsHere = (q == other.start);
                    bool otherEndsHere = (q == other.end);
                    bool thisStartsHere  = (p == start);
                    if ((thisEndsHere && otherStartsHere) || (otherEndsHere && thisStartsHere))
                        continue;
                    return true;
                }
            }
        }
        return false;
    }


    bool sharesPoint(int p) const {
        return std::find(points.begin(), points.end(), p) != points.end();
    }

    static bool comp(const LiveRange& lr1, const LiveRange& lr2){
        if (lr1.start != lr2.start) return lr1.start < lr2.start;
        return lr1.end < lr2.end;
    }
};
#endif //DAPROJECT2_LIVERANGE_H