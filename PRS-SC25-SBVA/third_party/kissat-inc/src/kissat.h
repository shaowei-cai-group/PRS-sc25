#ifndef _kissat_h_INCLUDED
#define _kissat_h_INCLUDED

#include "cvec.h"

typedef struct kissat kissat;

// Default (partial) IPASIR interface.

const char *kissat_signature (void);
kissat *kissat_init (void);
void kissat_add (kissat * solver, int lit);
int kissat_solve (kissat * solver);
void kissat_terminate (kissat * solver);
int kissat_value (kissat * solver, int lit);
void kissat_release (kissat * solver);

void kissat_set_terminate (kissat * solver,
			   void *state, int (*terminate) (void *state));

// Additional API functions.

void kissat_reserve (kissat * solver, int max_var);

const char *kissat_id (void);
const char *kissat_version (void);
const char *kissat_compiler (void);

void kissat_banner (const char *line_prefix, const char *name_of_app);

int kissat_get_option (kissat * solver, const char *name);
int kissat_set_option (kissat * solver, const char *name, int new_value);

void kissat_set_configuration (kissat * solver, const char *name);

void kissat_set_conflict_limit (kissat * solver, unsigned);
void kissat_set_decision_limit (kissat * solver, unsigned);

void kissat_print_statistics (kissat * solver);

// 导出学习子句的回调函数
typedef void (*prs_export_clause_callback)(void* state, cvec* clause, int glue);
void kissat_set_prs_export_clause_function(kissat *solver, 
    prs_export_clause_callback callback, void *state);

// 导入学习子句的回调函数
typedef int (*prs_import_clause_callback)(void* state, cvec* clause, int *glue);
void kissat_set_prs_import_clause_function(kissat *solver,
    prs_import_clause_callback callback, void *state);

// 决策变量的回调函数
typedef void (*prs_decide_callback)(void* state, int decide_lit);
void kissat_set_prs_decide_function(kissat *solver,
    prs_decide_callback callback, void *state);

// 设置PRS最佳相位
void kissat_set_prs_best_phase(kissat *solver, int* best_phase);

#endif
