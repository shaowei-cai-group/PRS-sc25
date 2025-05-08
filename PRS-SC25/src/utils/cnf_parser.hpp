#pragma once

#include <cstdio>
#include <vector>
#include <cstring>

extern "C" {
    #include "kissat.h"
}

char *read_whitespace(char *p) {                            // Aid function for parser
    while ((*p >= 9 && *p <= 13) || *p == 32) ++p;
    return p;
}

char *read_until_new_line(char *p) {                        // Aid function for parser
    while (*p != '\n') {
        if (*p++ == '\0') exit(1);
    }
    return ++p;
}

char *read_int(char *p, int *i) {                           // Aid function for parser
    bool sym = true; *i = 0;
    p = read_whitespace(p);
    if (*p == '-') sym = false, ++p;
    while (*p >= '0' && *p <= '9') {
        if (*p == '\0') return p;
        *i = *i * 10 + *p - '0', ++p;
    }
    if (!sym) *i = -(*i);
    return p;
}

void read_cnf(const char* filepath, int &num_vars, int &num_clauses, std::vector<std::vector<int>> &original_clauses) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        printf("Error opening file\n");
        exit(1);
    }

    const size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer
    char* buffer = new char[BUFFER_SIZE + 1];
    std::vector<int> clause_buffer;
    clause_buffer.reserve(1000); // Pre-allocate space for typical clause size
    
    char line[BUFFER_SIZE];
    bool header_found = false;
    
    while (fgets(line, sizeof(line), fp)) {
        char* p = line;
        p = read_whitespace(p);
        
        if (*p == 'c') continue;  // Skip comment lines
        
        if (*p == 'p' && !header_found) {  // Parse header
            if (strncmp(p, "p cnf", 5) == 0) {
                p += 5;
                p = read_int(p, &num_vars);
                p = read_int(p, &num_clauses);
                header_found = true;
                original_clauses.reserve(num_clauses); // Pre-allocate for expected number of clauses
            } else {
                printf("PARSE ERROR! Unexpected char\n");
                fclose(fp);
                delete[] buffer;
                exit(2);
            }
            continue;
        }
        
        // Parse clause data
        while (*p) {
            int32_t dimacs_lit;
            p = read_int(p, &dimacs_lit);
            if (!*p && dimacs_lit != 0) break;  // Incomplete line, continue with next line
            
            if (dimacs_lit == 0) {
                if (clause_buffer.empty()) {
                    printf("c PARSE ERROR! Empty Clause\n");
                    fclose(fp);
                    delete[] buffer;
                    exit(3);
                }
                original_clauses.push_back(clause_buffer);
                clause_buffer.clear();
            } else {
                clause_buffer.push_back(dimacs_lit);
            }
            p = read_whitespace(p);
        }
    }
    
    // Handle any remaining clause
    if (!clause_buffer.empty()) {
        original_clauses.push_back(clause_buffer);
    }
    
    delete[] buffer;
    fclose(fp);
}