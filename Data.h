#ifndef DAPROJECT2_DATA_H
#define DAPROJECT2_DATA_H

#include <string>
#include <vector>
#include "Web.h"
#include "RegisterPool.h"
#include "Graph.h"

/**
 * @brief Central data container passed through all pipeline stages.
 *
 * Holds the parsed webs, register pool, chosen algorithm and its parameter,
 * and the interference graph built from the webs.
 */
struct Data {
    std::vector<Web> webs;       ///< All webs derived from the input live ranges.
    RegisterPool pool;            ///< Register budget (K registers available).
    std::string algorithm;        ///< Algorithm to use: "basic", "spilling", "splitting", or "free".
    int algorithmPar = -1;        ///< Numeric parameter for spilling/splitting (max webs to spill/split).
    Graph<int> InterferenceGraph; ///< Interference graph; nodes are web IDs, edges indicate interference.
};

#endif //DAPROJECT2_DATA_H
