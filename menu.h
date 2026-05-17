/**
 * @file menu.h
 * @brief Interactive command-line menu for the register allocator tool.
 */

#ifndef DAPROJECT2_MENU_H
#define DAPROJECT2_MENU_H

#include <iostream>
#include <string>
#include "Parser.h"

/**
 * @brief Runs the interactive menu loop.
 *
 * Options:
 *  1 – Load input files, build webs and interference graph.
 *  2 – Run register allocation (algorithm auto-detected from config file).
 *  3 – Export results to a numbered .txt file under Output/.
 *  0 – Exit.
 *
 * @param parser A Parser instance used to read input files.
 */
void runMenu(Parser parser);

#endif //DAPROJECT2_MENU_H
