/**
 * @file Parser.cpp
 * @brief Implementation of Parser.
 */
//
// Created by afons on 5/11/2026.
//

#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <iostream>
#include "Parser.h"

std::string Parser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> Parser::split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delimiter))
        result.push_back(trim(token));
    return result;
}

void Parser::mergeRanges(Web& web) {
    if (web.ranges.size() < 2) return;

    bool merged = true;
    while (merged) {
        merged = false;
        for (int i = 0; i < (int)web.ranges.size() - 1; i++) {
            LiveRange& a = web.ranges[i];
            LiveRange& b = web.ranges[i + 1];
            if (a.end >= b.start) {
                for (const auto& p : b.points)
                    a.points.push_back(p);
                a.sortAndDedupe();
                a.recomputeBounds();
                web.ranges.erase(web.ranges.begin() + i + 1);
                merged = true;
                break;
            }
        }
    }
}

bool Parser::parseRegs(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    if (!file.is_open()) {
        std::cerr << "Error Cannot Open RegFile: " << fl << "." << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t marker = line.find(':');
        if (marker == std::string::npos) continue;

        std::string key = trim(line.substr(0, marker));
        std::string val = trim(line.substr(marker + 1));

        if (key == "registers") {
            data.pool = RegisterPool(std::stoi(val));
        }
        else if (key == "algorithm") {
            size_t comma = val.find(',');
            if (comma != std::string::npos) {
                data.algorithm = trim(val.substr(0, comma));
                data.algorithmPar = std::stoi(trim(val.substr(comma + 1)));
            } else {
                data.algorithm = trim(val);
            }
        }
    }
    return true;
}

bool Parser::parseRanges(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    if (!file.is_open()) {
        std::cerr << "Error Cannot Open RangeFile: " << fl << "." << std::endl;
        return false;
    }

    std::map<std::string, int> webIndex;
    int webIdCount = 0;
    int rangeIdCount = 0;

    std::map<std::string, bool>              inRangeMap;
    std::map<std::string, std::vector<Point>> ptsMap;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos)
            line = trim(line.substr(0, hashPos));
        if (line.empty()) continue;

        size_t marker = line.find(':');
        if (marker == std::string::npos) continue;

        std::string name = trim(line.substr(0, marker));
        std::string rest = trim(line.substr(marker + 1));

        if (webIndex.find(name) == webIndex.end()) {
            webIndex[name] = (int)data.webs.size();
            data.webs.emplace_back(webIdCount++, name);
            inRangeMap[name] = false;
            ptsMap[name] = {};
        }
        Web& web = data.webs[webIndex[name]];

        bool& inRange = inRangeMap[name];
        std::vector<Point>& pts = ptsMap[name];

        auto tokens = split(rest, ',');

        for (const auto& t : tokens) {
            if (t.empty()) continue;
            char op = t.back();

            if (op == '+') {
                if (inRange && !pts.empty()) {
                    web.addRange(LiveRange(rangeIdCount++, name, pts));
                    pts.clear();
                }
                int lineNum = std::stoi(t.substr(0, t.size() - 1));
                pts.push_back(Point(lineNum, true, false));
                inRange = true;
            }
            else if (op == '-') {
                int lineNum = std::stoi(t.substr(0, t.size() - 1));
                pts.push_back(Point(lineNum, false, true));
                web.addRange(LiveRange(rangeIdCount++, name, pts));
                pts.clear();
                inRange = false;
            }
            else {
                int lineNum = std::stoi(t);
                pts.push_back(Point(lineNum, false, false));
                inRange = true;
            }
        }
    }

    for (auto& w : data.webs) {
        w.sortRange();
        mergeRanges(w);
        w.rebuildPoints();
    }

    return true;
}

Data Parser::parse(const std::string& rangesFile, const std::string& regsFile, bool& regdone, bool& rangesdone) {
    Data data;
    if (!parseRegs(regsFile, data))
        regdone = false;
    if (!parseRanges(rangesFile, data))
        rangesdone = false;
    return data;
}