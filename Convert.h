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
    static void allocate(Data& data);
    static void allocateSpilling(Data& data);
    static void allocateSplitting(Data& data);
    static int Degree(const Web& web, const std::vector<Web>& webs);
    static int findIdx(int id, const std::vector<Web>& webs);
    static void writeOutput(const Data& data, const std::string& outputFile);
    static void printResults(const Data& data);
private:
    static bool tryColor(Data& data);
};

#endif //DAPROJECT2_CONVERT_H