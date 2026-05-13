#include <iostream>
#include <string>
#include <vector>
#include "Parser.h"
#include "menu.h"

int main(int argc, char* argv[]) {
    Parser parser;

    // batch mode
    if (argc > 1 && std::string(argv[1]) == "-b") {
        if (argc < 5) {
            std::cerr << "Error: Missing arguments for batch mode." << std::endl;
            return 1;
        }
        std::string Ranges = argv[2];
        std::string Registers = argv[3];
        std::string outputFile = argv[4];

        std::cout << "Running in batch mode with " << Ranges << "and" << Registers << "..." << std::endl;
        return 0;
    }
    //menu mode
    runMenu(parser);
    return 0;
}