
#ifndef DAPROJECT2_POINT_H
#define DAPROJECT2_POINT_H

#include <string>
#include <vector>
#include <algorithm>


struct Point {
    int  line;
    bool isDef;
    bool isKill;

    Point(int l, bool d, bool k) : line(l), isDef(d), isKill(k) {}

    void absorb(const Point& o) {
        isDef  = isDef  || o.isDef;
        isKill = isKill || o.isKill;
    }

    bool operator<(const Point& o) const { return line < o.line; }
};

#endif //DAPROJECT2_POINT_H