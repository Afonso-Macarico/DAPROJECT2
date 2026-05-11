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

    Web(int id, std::string name) : id(id), Name(name), reg(-1), overflow(false) {}

    void addRange(const LiveRange& lr){ ranges.push_back(lr);}
    void sortRange(){std::sort(ranges.begin(),ranges.end(), LiveRange::comp);}
};
#endif //DAPROJECT2_WEB_H