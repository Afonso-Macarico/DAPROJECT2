#include "menu.h"
#include "Convert.h"

void runMenu(Parser parser) {
    int choice = -1;
    Data data;
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
                std::string Ranges;
                std::cout << "Enter the Ranges file path: ";
                std::getline(std::cin >> std::ws, Ranges);

                if (!Ranges.empty() && Ranges.front() == '"' && Ranges.back() == '"') {
                    Ranges = Ranges.substr(1, Ranges.size() - 2);
                }

                std::string Registers;
                std::cout << "Enter the Registers file path: ";
                std::getline(std::cin >> std::ws, Registers);

                if (!Registers.empty() && Registers.front() == '"' && Registers.back() == '"') {
                    Registers = Registers.substr(1, Registers.size() - 2);
                }
                bool regdone=true;
                bool rangedone=true;
                Data temp = parser.parse(Ranges, Registers,regdone ,rangedone);
                if (!regdone or !rangedone) {
                    std::cerr << "Press Enter to continue"<<std::endl;
                    std::cin.get();
                    break;
                }
                temp.InterferenceGraph = Convert::BuildGraph(temp);
                data = temp;
                //Checking if the Graph built

                std::cout << "\nDataset loaded successfully."<< std::endl<< "Press Enter to continue...";
                std::cin.get();
                break;
            }
            case 2: {
                /**if (data.submissions.empty()) {
                    std::cerr << "Error: No data in memory. Please load a file first (Option 1)." << std::endl<< "Press Enter to continue...";
                    std::cin.ignore(1000, '\n');
                    std::cin.get();
                } else {
                    std::cout << "TODO..." << std::endl;
                    std::cin.ignore(1000, '\n');
                    std::cout << "\nSuccessfully. Press Enter to continue...";
                    std::cin.get();
                }**/
                break;
            }
            case 3:
                /**if (data.submissions.empty()) {
                    std::cerr << "Error: No data in memory. Please load a file first (Option 1)." << std::endl<< "Press Enter to continue...";
                    std::cin.ignore(1000, '\n');
                    std::cin.get();
                }
                else {
                    std::cout << "TODO....." << std::endl;
                    std::cin.ignore(1000, '\n');
                    std::cout << "\nSuccessfully. Press Enter to continue...";
                    std::cin.get();
                }**/
                break;
            case 0:
                std::cout << "Exiting program. Goodbye!" << std::endl;
                break;
            default:
                std::cout << "Option not recognized." << std::endl;
        }
    }
}