/**
 * @file Parser.h
 * @brief Parses live-range and register configuration input files.
 */

#ifndef DAPROJECT2_PARSER_H
#define DAPROJECT2_PARSER_H

#include "Data.h"
#include <vector>
#include <string>

/**
 * @brief Reads and parses the two input files, building the Data structure.
 */
class Parser {
public:
    /**
     * @brief Parses both input files and returns a populated Data object.
     * @param rangesFile   Path to the live-ranges text file.
     * @param regsFile     Path to the registers/algorithm config file.
     * @param regdone      Set to false if the registers file could not be opened.
     * @param rangesdone   Set to false if the ranges file could not be opened.
     * @return Populated Data with webs, pool and algorithm settings.
     * @complexity O(L * p log p) where L = number of input lines, p = points per web.
     */
    Data parse(const std::string& rangesFile, const std::string& regsFile, bool& regdone, bool& rangesdone);

private:
    /**
     * @brief Parses the ranges file and populates data.webs.
     * @complexity O(L * p log p)
     */
    bool parseRanges(const std::string& filename, Data& data);

    /**
     * @brief Parses the registers file for K and algorithm settings.
     * @complexity O(L)
     */
    bool parseRegs(const std::string& filename, Data& data);

    /**
     * @brief Merges touching or overlapping LiveRanges within a Web.
     *
     * Two ranges are merged when the end line of one equals or exceeds
     * the start line of the next (e.g. i=i+1 creates adjacent ranges).
     * @complexity O(r^2) in the worst case, where r = number of ranges in the web.
     */
    void mergeRanges(Web& web);

    /// @brief Splits string s on delimiter and trims each token. @complexity O(n)
    std::vector<std::string> split(const std::string& s, char delimiter);

    /// @brief Removes leading/trailing whitespace. @complexity O(n)
    std::string trim(const std::string& str);
};

#endif //DAPROJECT2_PARSER_H
