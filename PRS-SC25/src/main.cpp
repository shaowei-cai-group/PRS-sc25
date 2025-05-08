#include <cstdio>
#include <thread>
#include <atomic>
#include <optional>
#include <future>
#include <chrono>

#include "preprocess/preprocess.hpp"

extern "C" {
    #include "kissat.h"
    #include "cvec.h"
}

#include "prs/prs.hpp"
#include "prs/options.hpp"

int main(int argc, char *argv[]) {
    INIT_ARGS(argc, argv);
    PRINT_ARGS();
    PRS *prs = new PRS();

    int res;

    if(OPT(mode) == 0) {
        printf("c mode PRS\n");
        res = prs->solve(OPT(filename).c_str());
    } else {
        printf("c mode SBVA\n");
        res = prs->mix_solve(OPT(filename).c_str());
    }

    if(res == 10) {
        printf("s SATISFIABLE\n");
        vec<int> &model = prs->getModel();
        printf("v");
        for (int i = 0; i <model.size(); i++) {
            printf(" %d", model[i]);
        }
        puts(" 0");
    } else if(res == 20) {
        printf("s UNSATISFIABLE\n");
    } else {
        printf("s UNKNOWN\n");
    }

    return 0;
}