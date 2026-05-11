#include <fstream>
#include <sstream>
#include "Parser.h"
#include <map>

void Parser::parseRegs(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    std::string line;
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
            auto alg= split(val,' ');
            data.algorithm=alg[0];
            if (alg.size()>1)
                data.algorithmPar=std::stoi(alg[1]);
        }
    }
}

void Parser::parseRanges(const std::string& fl, Data& data) {
    std::ifstream file(fl);
    std::map<std::string, int> webIndex;
    int webIdCount=0;
    int rangeIdCount=0;
    std::string line;
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
        auto tokens = split(rest, ',');
        int start=-1;
        for (const auto& t : tokens) {
            if (t.empty()) continue;
            char op= t.back();
            if (op == '+') {
                start=std::stoi(t.substr(0, t.size()-1));
            }
            else if (op == '-') {
                int end=std::stoi(t.substr(0, t.size()-1));
                web.addRange(LiveRange(rangeIdCount, name, start, end));
                rangeIdCount++;
                start=-1;
            }
        }
    }
        for (auto& w: data.webs) w.sortRange();
}

Data Parser::parse(const std::string& rangesFile, const std::string& regsFile) {
    Data data;
    parseRegs(regsFile,data);
    parseRanges(rangesFile, data);
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
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, (last - first + 1));
}