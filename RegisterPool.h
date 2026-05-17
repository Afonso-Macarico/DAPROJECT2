#ifndef DAPROJECT2_REGISTERPOOL_H
#define DAPROJECT2_REGISTERPOOL_H

#include <string>

/**
 * @brief Holds the register budget and provides register name formatting.
 *
 * The pool simply stores K (the maximum number of physical registers available)
 * and maps integer register indices to names such as "r0", "r1", etc.
 */
class RegisterPool {
private:
    int K; ///< Maximum number of available physical registers.

public:
    /// @brief Default constructor (K = 0). @complexity O(1)
    RegisterPool() : K(0) {}

    /// @brief Constructs a pool with k registers. @complexity O(1)
    RegisterPool(int k) : K(k) {}

    /// @brief Returns the number of available registers. @complexity O(1)
    int getK() const { return K; }

    /**
     * @brief Returns the name of register id (e.g. "r0"), or "SPILL" for id == -1.
     * @complexity O(1)
     */
    std::string getRegisterName(int id) const {
        if (id == -1) return "SPILL";
        return "R" + std::to_string(id);
    }
};

#endif //DAPROJECT2_REGISTERPOOL_H
