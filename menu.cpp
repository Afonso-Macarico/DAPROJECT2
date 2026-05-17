#include "menu.h"
#include "Convert.h"
#include <filesystem>

void runMenu(Parser parser) {
    int choice = -1;
    Data data;
    std::string currentRangesName; // used to name the output file

    while (choice != 0) {
        std::cout << "\n==========================================" << std::endl;
        std::cout << "   Compiler Register Allocator   " << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "1. Initialize System (Load, Build Webs & Graph)" << std::endl;
        std::cout << "2. Perform Register Allocation (Auto-detect Algorithm)" << std::endl;
        std::cout << "3. Export Allocation Results (.txt)" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Choice: ";

        if (!(std::cin >> choice)) {
            std::cerr << "Invalid input! Please enter a number." << std::endl;
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            choice = -1;
            std::cerr << "Press Enter to return to menu...";
            std::cin.get();
            continue;
        }

        switch (choice) {
            case 1: {
                std::string rangesFile;
                std::cout << "Enter ranges filename (in Data/): ";
                std::cin >> rangesFile;
                if (rangesFile.size() < 4 || rangesFile.substr(rangesFile.size() - 4) != ".txt")
                    rangesFile += ".txt";

                std::string registersFile;
                std::cout << "Enter registers filename (in Data/): ";
                std::cin >> registersFile;
                if (registersFile.size() < 4 || registersFile.substr(registersFile.size() - 4) != ".txt")
                    registersFile += ".txt";

                std::string Ranges    = "../Data/" + rangesFile;
                std::string Registers = "../Data/" + registersFile;

                bool regdone   = true;
                bool rangedone = true;
                Data temp = parser.parse(Ranges, Registers, regdone, rangedone);
                if (!regdone || !rangedone) {
                    std::cerr << "Failed to load files. Check they exist in Data/." << std::endl<<"Press Enter to continue...";
                    std::cin.ignore(1000, '\n');
                    std::cin.get();
                    break;
                }
                temp.InterferenceGraph = Convert::BuildGraph(temp);
                data = temp;

                // derive output name from ranges filename (strip extension)
                currentRangesName = rangesFile;
                size_t dot = currentRangesName.rfind('.');
                if (dot != std::string::npos) currentRangesName = currentRangesName.substr(0, dot);

                std::cout << "\nLoaded successfully." << std::endl;
                std::cout << "  Ranges:    " << Ranges    << std::endl;
                std::cout << "  Registers: " << Registers << std::endl;
                std::cout << "Press Enter to continue...";
                std::cin.ignore(1000, '\n');
                std::cin.get();
                break;
            }
            case 2: {
                if (data.webs.empty()) {
                    std::cerr << "Error: No data loaded. Please load files first (Option 1)." << std::endl;
                    std::cin.ignore(1000, '\n');
                    std::cin.get();
                    break;
                }
                Convert::allocate(data);
                std::cout << "\nAllocation complete. Results:\n\n";
                Convert::printResults(data);
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore(1000, '\n');
                std::cin.get();
                break;
            }
            case 3: {
                if (data.webs.empty()) {
                    std::cerr << "Error: No data loaded. Please load files first (Option 1)." << std::endl;
                    std::cin.ignore(1000, '\n');
                    std::cin.get();
                    break;
                }
                std::filesystem::create_directories("../Output");
                // find next available allocation number
                int x = 1;
                while (std::filesystem::exists("../Output/allocation_" + std::to_string(x) + ".txt"))
                    x++;
                std::string outPath = "../Output/allocation_" + std::to_string(x) + ".txt";
                Convert::writeOutput(data, outPath);
                std::cout << "Results written to " << outPath << "\nPress Enter to continue...";
                std::cin.ignore(1000, '\n');
                std::cin.get();
                break;
            }
            case 0:
                std::cout << "Exiting program. Goodbye!" << std::endl;
                break;
            default:
                std::cout << "Option not recognized." << std::endl;
        }
    }
}