#include <fstream>
#include <sstream>
#include "Parser.h"
#include <map>
#include <iostream>
#include <algorithm>

bool Parser::parseRegs(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    std::string line;
    if (!file.is_open()) {
        std::cerr<<"Error Cannot Open RegFile: "<<fl<<"."<< std::endl;
        return false;
    }
    while (std::getline(file, line)) {
        line=trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t marker=line.find(':');
        if (marker==std::string::npos) continue;
        std::string key= trim(line.substr(0, marker));
        std::string val= trim(line.substr(marker+1));

        if (key=="registers") {
            data.pool = RegisterPool(std::stoi(val));
        }
        else if (key=="algorithm") {
            // format is either "basic" or "spilling, 2" or "splitting, 2"
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
    std::map<std::string, int> webIndex;
    int webIdCount=0;
    int rangeIdCount=0;
    std::string line;
    if (!file.is_open()) {
        std::cerr<<"Error Cannot Open RangeFile: "<<fl<<"."<< std::endl;
        return false;
    }
    while (std::getline(file, line)) {
        line=trim(line);
        if (line.empty() || line[0] =='#') continue;
        size_t marker=line.find(':');
        if (marker== std::string::npos) continue;
        std::string name= trim(line.substr(0, marker));
        std::string rest=trim(line.substr(marker+1));

        if (webIndex.find(name)==webIndex.end()) {
            webIndex[name]=(int)data.webs.size();
            data.webs.emplace_back(webIdCount,name);
            webIdCount++;
        }
        Web& web=data.webs[webIndex[name]];

        // collect all program points in this line, preserving +/- markers
        auto tokens = split(rest, ',');
        std::vector<int> points;
        int start=-1;
        int end=-1;
        bool hasStart = false;
        bool hasEnd = false;

        for (const auto& t : tokens) {
            if (t.empty()) continue;
            char op = t.back();
            int lineNum;
            if (op == '+') {
                lineNum = std::stoi(t.substr(0, t.size()-1));
                if (!hasStart) { start = lineNum; hasStart = true; }
                points.push_back(lineNum);
            } else if (op == '-') {
                lineNum = std::stoi(t.substr(0, t.size()-1));
                end = lineNum;
                hasEnd = true;
                points.push_back(lineNum);
            } else {
                lineNum = std::stoi(t);
                points.push_back(lineNum);
            }
        }

        if (hasStart && hasEnd) {
            web.addRange(LiveRange(rangeIdCount, name, start, end, points));
            rangeIdCount++;
        }
    }

    // merge live ranges of the same variable that share any program point,
    // or where one ends on the same line another starts (the i=i+1 edge case)
    for (auto& w : data.webs) {
        mergeRanges(w);
    }

    for (auto& w: data.webs) w.sortRange();

    // build interference graph: one node per web, edge if any ranges interfere
    for (auto& w : data.webs) {
        data.InterferenceGraph.addVertex(w.id);
    }
    for (int i = 0; i < (int)data.webs.size(); i++) {
        for (int j = i+1; j < (int)data.webs.size(); j++) {
            bool interfere = false;
            for (auto& ra : data.webs[i].ranges) {
                for (auto& rb : data.webs[j].ranges) {
                    if (ra.overlaps(rb)) { interfere = true; break; }
                }
                if (interfere) break;
            }
            if (interfere) {
                data.InterferenceGraph.addEdge(data.webs[i].id, data.webs[j].id, 1);
                data.InterferenceGraph.addEdge(data.webs[j].id, data.webs[i].id, 1);
            }
        }
    }

    return true;
}

void Parser::mergeRanges(Web& web) {
    bool merged = true;
    while (merged) {
        merged = false;
        for (int i = 0; i < (int)web.ranges.size() && !merged; i++) {
            for (int j = i+1; j < (int)web.ranges.size() && !merged; j++) {
                LiveRange& a = web.ranges[i];
                LiveRange& b = web.ranges[j];

                // check if they share any point OR if one ends where the other starts
                bool shouldMerge = (a.end == b.start) || (b.end == a.start);
                if (!shouldMerge) {
                    for (int p : a.points) {
                        for (int q : b.points) {
                            if (p == q) { shouldMerge = true; break; }
                        }
                        if (shouldMerge) break;
                    }
                }

                if (shouldMerge) {
                    // merge b into a: union of all points, update start/end
                    for (int p : b.points) {
                        if (std::find(a.points.begin(), a.points.end(), p) == a.points.end())
                            a.points.push_back(p);
                    }
                    std::sort(a.points.begin(), a.points.end());
                    a.start = a.points.front();
                    a.end   = a.points.back();
                    web.ranges.erase(web.ranges.begin() + j);
                    merged = true;
                }
            }
        }
    }
}

Data Parser::parse(const std::string& rangesFile, const std::string& regsFile, bool& regdone, bool& rangesdone) {
    Data data;
    if (!parseRegs(regsFile,data)) {
        regdone=false;
    }
    if (!parseRanges(rangesFile, data)) {
        rangesdone=false;
    }
    return data;
}

//aux functions

std::vector<std::string> Parser::split(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(trim(token));
    }
    return result;
}

std::string Parser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}