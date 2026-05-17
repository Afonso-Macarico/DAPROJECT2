//
// Created by afons on 5/11/2026.
//

#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <iostream>
#include "Parser.h"

// ─────────────────────────────────────────────
//  Utilities
// ─────────────────────────────────────────────

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

// ─────────────────────────────────────────────
//  mergeRanges
//  Merges LiveRanges within a web that touch or
//  overlap (e.g. one ends at 12, next starts at 12)
// ─────────────────────────────────────────────

void Parser::mergeRanges(Web& web) {
    if (web.ranges.size() < 2) return;

    // requires sorted input — sortRange() must be called before this
    bool merged = true;
    while (merged) {
        merged = false;
        for (int i = 0; i < (int)web.ranges.size() - 1; i++) {
            LiveRange& a = web.ranges[i];
            LiveRange& b = web.ranges[i + 1];
            // merge if touching (a.end == b.start) or overlapping (a.end > b.start)
            if (a.end >= b.start) {
                // Combine all points from b into a, then recompute a's bounds
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

// ─────────────────────────────────────────────
//  parseRegs
//  Reads: registers: N
//         algorithm: basic | spilling, K | splitting, K | free
// ─────────────────────────────────────────────

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
            // format: "basic" or "spilling, 2" or "splitting, 2"
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

// ─────────────────────────────────────────────
//  parseRanges
//  Reads: name: N+, N, N, N-  (one LiveRange per line)
//  Same name may appear on multiple lines → same Web
//  After parsing: sort then merge touching/overlapping ranges
// ─────────────────────────────────────────────

bool Parser::parseRanges(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    if (!file.is_open()) {
        std::cerr << "Error Cannot Open RangeFile: " << fl << "." << std::endl;
        return false;
    }

    std::map<std::string, int> webIndex;
    int webIdCount = 0;
    int rangeIdCount = 0;

    // per-web carry-over state for ranges that span multiple input lines
    std::map<std::string, bool>              inRangeMap;
    std::map<std::string, std::vector<Point>> ptsMap;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        // strip inline comments
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos)
            line = trim(line.substr(0, hashPos));
        if (line.empty()) continue;

        size_t marker = line.find(':');
        if (marker == std::string::npos) continue;

        std::string name = trim(line.substr(0, marker));
        std::string rest = trim(line.substr(marker + 1));

        // find or create web
        if (webIndex.find(name) == webIndex.end()) {
            webIndex[name] = (int)data.webs.size();
            data.webs.emplace_back(webIdCount++, name);
            inRangeMap[name] = false;
            ptsMap[name] = {};
        }
        Web& web = data.webs[webIndex[name]];

        // carry over state from previous lines for this variable
        bool& inRange = inRangeMap[name];
        std::vector<Point>& pts = ptsMap[name];

        auto tokens = split(rest, ',');

        for (const auto& t : tokens) {
            if (t.empty()) continue;
            char op = t.back();

            if (op == '+') {
                // flush any open range first
                if (inRange && !pts.empty()) {
                    web.addRange(LiveRange(rangeIdCount++, name, pts));
                    pts.clear();
                }
                int lineNum = std::stoi(t.substr(0, t.size() - 1));
                pts.push_back(Point(lineNum, /*isDef=*/true, /*isKill=*/false));
                inRange = true;
            }
            else if (op == '-') {
                int lineNum = std::stoi(t.substr(0, t.size() - 1));
                pts.push_back(Point(lineNum, /*isDef=*/false, /*isKill=*/true));
                web.addRange(LiveRange(rangeIdCount++, name, pts));
                pts.clear();
                inRange = false;
            }
            else {
                // plain intermediate point — add and mark as in range
                int lineNum = std::stoi(t);
                pts.push_back(Point(lineNum, false, false));
                inRange = true;
            }
        }
        // do NOT warn here — range may continue on the next line
    }

    // sort then merge touching/overlapping ranges within each web
    for (auto& w : data.webs) {
        w.sortRange();
        mergeRanges(w);
        w.rebuildPoints();  // keep allPoints in sync after merges
    }

    return true;
}

// ─────────────────────────────────────────────
//  Public entry point
// ─────────────────────────────────────────────

Data Parser::parse(const std::string& rangesFile, const std::string& regsFile, bool& regdone, bool& rangesdone) {
    Data data;
    if (!parseRegs(regsFile, data))
        regdone = false;
    if (!parseRanges(rangesFile, data))
        rangesdone = false;
    return data;
}