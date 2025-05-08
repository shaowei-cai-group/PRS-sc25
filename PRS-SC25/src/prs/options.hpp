#pragma once

#include <string>
#include <cstring>
#include <unordered_map>

#include "utils/cmdline.h"

//      name              , type    , short , must   , default , low  , high   , comments 
#define OPTIONS \
OPTION( cutoff            , double  , '\0'  , false  , 5000.0  , 0.0  , 1e18    , "cutoff time(seconds)") \
OPTION( threads           , int     , 't'   , false  , 32      , 0    , 256     , "threads") \
OPTION( yalsat            , int     , 'y'   , false  , 1       , 0    , 1       , "use yalsat solver") \
OPTION( share_lits        , int     , '\0'  , false  , 1500    , 0    , 1e18    , "shared lits limit per thread per share_intv") \
OPTION( share_intv        , int     , '\0'  , false  , 500     , 0    , 1e18    , "share interval(miliseconds)") \
OPTION( share_grps        , int     , '\0'  , false  , 4       , 1    , 256     , "max share group size") \
OPTION( mode              , int     , '\0'  , true   , 0       , 0    , 1       , "0 for PRS, 1 for SBVA")

class Options
{
public:

std::string filename;

#define OPTION(N, T, S, M, D, L, H, C) \
    T N = D;
    OPTIONS 
#undef OPTION

void parse_args(int argc, char *argv[]) {
    cmdline::parser parser;

    #define OPTION(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) parser.add<int>(#N, S, C, M, D, cmdline::range((int)L, (int)H)); \
    if (!strcmp(#T, "double")) parser.add<double>(#N, S, C, M, D, cmdline::range((double)L, (double)H));
    OPTIONS
    #undef OPTION

    parser.footer("filename");
    parser.parse_check(argc, argv);

    if(parser.rest().size() == 0) {
        printf("Error: filename is required\n");
        exit(1);
    }

    filename = parser.rest()[0];

    #define OPTION(N, T, S, M, D, L, H, C) \
    if (!strcmp(#T, "int")) N = parser.get<int>(#N); \
    if (!strcmp(#T, "double")) N = parser.get<double>(#N);
    OPTIONS
    #undef OPTION
}
void print_change() {
    printf("c ----------------------------------------- Paras list -------------------------------------------------\n");
    printf("c %-15s\t %-8s\t %-10s\t %-10s\t %s\n",
           "Name", "Type", "Now", "Default", "Comment");

#define OPTION(N, T, S, M, D, L, H, C) \
    if (strcmp(#T, "int") == 0) printf("c %-15s\t %-8s\t %-10d\t %-10s\t %s\n", (#N), (#T), (int)this->N, (#D), (C)); \
    if (strcmp(#T, "double") == 0) printf("c %-15s\t %-8s\t %-10.2f\t %-10s\t %s\n", (#N), (#T), (double)this->N, (#D), (C));
    OPTIONS
#undef OPTION
    printf("c -----------------------------------------------------------------------------------------------------\n");
}
};

inline Options __global_options;

inline void INIT_ARGS(int argc, char **argv) {
    __global_options.parse_args(argc, argv);
}

inline void PRINT_ARGS() {
    __global_options.print_change();
}

#define OPT(opt) (__global_options.opt)