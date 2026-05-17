/**
 * @file Convert.h
 * @brief Builds the interference graph and runs register allocation algorithms.
 */

#ifndef DAPROJECT2_CONVERT_H
#define DAPROJECT2_CONVERT_H

#include "Data.h"
#include "Graph.h"

/**
 * @brief Static utility class for interference graph construction and register allocation.
 *
 * All allocation algorithms follow the same greedy graph-colouring approach:
 * a simplification phase removes nodes of degree < K onto a stack, then a
 * colouring phase pops them back and assigns the lowest available colour.
 */
class Convert {
public:
    /**
     * @brief Builds the interference graph from the webs in data.
     *
     * Adds one vertex per web (keyed by web.id) and an undirected edge between
     * any two webs whose live ranges overlap (interfere).
     * @complexity O(W^2 * p) where W = number of webs, p = average points per web.
     */
    static Graph<int> BuildGraph(const Data& data);

    /**
     * @brief Dispatches to the correct allocation algorithm based on data.algorithm.
     *
     * Supported values: "basic", "spilling", "splitting", "free".
     * @complexity Depends on the selected algorithm (see individual methods).
     */
    static void allocate(Data& data);

    /**
     * @brief Greedy colouring with up to K web spills to resolve conflicts.
     *
     * Iteratively spills the highest-degree web (tiebreak: most points) and
     * retries colouring until success or the spill budget is exhausted.
     * @complexity O(K * W^3 * p)
     */
    static void allocateSpilling(Data& data);

    /**
     * @brief Greedy colouring with up to K web splits to reduce interference.
     *
     * Splits the highest-degree web at the gap with the fewest cross-web
     * interferences, creating two smaller derived webs, then retries colouring.
     * @complexity O(K * W^3 * p)
     */
    static void allocateSplitting(Data& data);

    /**
     * @brief Linear-scan register allocation (interval-based, alternative approach).
     *
     * Sorts live ranges by start point and greedily assigns the first free
     * register. When no register is free the range with the furthest endpoint
     * is evicted (spilled).
     * @complexity O(W * r * log(W * r)) where r = average ranges per web.
     */
    static void allocateFree(Data& data);

    /**
     * @brief Returns the number of active neighbours of web in the current graph state.
     * @complexity O(W * p)
     */
    static int Degree(const Web& web, const std::vector<Web>& webs);

    /**
     * @brief Returns the index in webs of the web with the given id, or -1.
     * @complexity O(W)
     */
    static int findIdx(int id, const std::vector<Web>& webs);

    /**
     * @brief Writes allocation results to outputFile (or stdout if empty).
     * @complexity O(W)
     */
    static void writeOutput(const Data& data, const std::string& outputFile);

    /// @brief Prints allocation results to stdout. @complexity O(W)
    static void printResults(const Data& data);

private:
    /**
     * @brief Runs only the colouring phase (no external spill decisions).
     *
     * Returns true if all non-overflow webs were successfully coloured.
     * Used internally by allocateSpilling and allocateSplitting.
     * @complexity O(W^3 * p)
     */
    static bool tryColor(Data& data);
};

#endif //DAPROJECT2_CONVERT_H
