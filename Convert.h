//
// Created by afons on 5/17/2026.
//

#ifndef DAPROJECT2_CONVERT_H
#define DAPROJECT2_CONVERT_H

#include "Data.h"
#include "Graph.h"

class Convert {
public:
    static Graph<int> BuildGraph(const Data& data);
};

#endif //DAPROJECT2_CONVERT_H