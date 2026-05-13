//
// Created by afons on 5/11/2026.
//

#ifndef DAPROJECT2_PARSER_H
#define DAPROJECT2_PARSER_H

#include "Data.h"
#include <vector>
#include <string>

class Parser {
public:
    Data parse(const std::string& rangesFile, const std::string& regsFile, bool& regdone,bool& rangesdone);

private:
    bool parseRanges(const std::string& filename, Data& data);
    bool parseRegs(const std::string& filename, Data& data);

    std::vector<std::string> split(const std::string& s, char delimiter);
    std::string trim(const std::string& str);
};
#endif //DAPROJECT2_PARSER_H