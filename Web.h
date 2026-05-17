/**
* @file Web.h
 * @brief Defines the Web data structure.
 */

#ifndef DAPROJECT2_WEB_H
#define DAPROJECT2_WEB_H
#include <string>
#include <vector>
#include <algorithm>
#include "LiveRange.h"

struct Web {
    int id;
    std::string Name;
    std::vector<LiveRange> ranges;
    int reg;
    bool overflow;
    bool active;

    Web(int id, std::string name) : id(id), Name(name), reg(-1), overflow(false), active(true) {}

    void addRange(const LiveRange& lr){ ranges.push_back(lr);}
    void sortRange(){std::sort(ranges.begin(),ranges.end(), LiveRange::comp);}
    bool OverlapCheck(const Web& other) const {
        for (const auto& r1 : ranges)
            for (const auto& r2 : other.ranges)
                if (r1.overlaps(r2)) return true;
        return false;
    }
    void mergeRanges() {
        if (ranges.size() < 2) return;
        bool merged = true;
        while (merged) {
            merged = false;
            for (int i = 0; i < (int)ranges.size() - 1; i++) {
                if (ranges[i].overlaps(ranges[i + 1])) {
                    ranges[i].end = std::max(ranges[i].end, ranges[i + 1].end);
                    ranges.erase(ranges.begin() + i + 1);
                    merged = true;
                    break;
                }
            }
        }
    }
};
#endif //DAPROJECT2_WEB_H