/**
* @file LiveRange.h
 * @brief Defines the LiveRange data structure.
 */

#ifndef DAPROJECT2_LIVERANGE_H
#define DAPROJECT2_LIVERANGE_H
#include <string>

struct LiveRange {
    int id;
    std::string Name;
    int start;
    int end;

    LiveRange(int id, std::string name, int start, int end)
        : id(id), Name(name), start(start), end(end) {}

    bool overlaps(const LiveRange& other) const {
        return (start <= other.end && other.start <= end);
    }

    static bool comp(const LiveRange& lr1, const LiveRange& lr2) {
        if (lr1.start != lr2.start) return lr1.start < lr2.start;
        return lr1.end < lr2.end;
    }
};

#endif //DAPROJECT2_LIVERANGE_H