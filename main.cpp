/**
 * @file main.cpp
 * @brief Entry point for the Compiler Register Allocator tool.
 *
 * Supports two modes:
 *  - Batch mode:       ./myProg -b ranges.txt registers.txt allocation.txt
 *  - Interactive mode: ./myProg  (launches the menu)
 */
#include <iostream>
#include <string>
#include <vector>
#include "Parser.h"
#include "menu.h"
#include "Convert.h"

int main(int argc, char* argv[]) {
    Parser parser;

    if (argc > 1 && std::string(argv[1]) == "-b") {
        if (argc < 5) {
            std::cerr << "Error: Missing arguments for batch mode." << std::endl;
            return 1;
        }
        std::string Ranges = argv[2];
        std::string Registers = argv[3];
        std::string outputFile = argv[4];

        bool regdone = true;
        bool rangesdone = true;
        Data data = parser.parse(Ranges, Registers, regdone, rangesdone);
        if (!regdone || !rangesdone) {
            std::cerr << "Parsing failed." << std::endl;
            return 1;
        }
        data.InterferenceGraph = Convert::BuildGraph(data);
        Convert::allocate(data);
        Convert::writeOutput(data, outputFile);
        return 0;
    }
    runMenu(parser);
    return 0;
}