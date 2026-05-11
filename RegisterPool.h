//
// Created by afons on 5/11/2026.
//

#ifndef DAPROJECT2_REGISTERPOOL_H
#define DAPROJECT2_REGISTERPOOL_H

#include <string>

class RegisterPool {
private:
    int K;

public:
    RegisterPool(int k) : K(k) {}
    int getK() const { return K; }
    std::string getRegisterName(int id) const {
        if (id == -1) return "SPILL";
        return "R" + std::to_string(id);
    }
};

#endif //DAPROJECT2_REGISTERPOOL_H