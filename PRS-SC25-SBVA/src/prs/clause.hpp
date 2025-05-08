#pragma once

#include "utils/vec.hpp"

class Clause {
public:
    Clause(int lbd, int size) : lbd(lbd), size(size){
        literals = new int[size];
    }
    ~Clause() {
        delete[] literals;
    }
    int lbd;
    int size;
    int *literals;
};