//
// Created by afons on 5/11/2026.
//

#ifndef DAPROJECT2_DATA_H
#define DAPROJECT2_DATA_H

#include <string>
#include <vector>
#include "Web.h"
#include "RegisterPool.h"
#include "Graph.h"

struct Data {
    std::vector<Web> webs;
    RegisterPool pool;
    std::string algorithm;
    int algorithmPar = -1;
    Graph<int> InterferenceGraph;
};
#endif //DAPROJECT2_DATA_H