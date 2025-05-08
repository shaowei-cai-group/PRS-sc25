#include "internal.h"
#include "inline.h"

void kissat_set_prs_export_clause_function(kissat *solver, 
    prs_export_clause_callback callback, void *state) {
    assert(callback != NULL);
    solver->prsExportClause = callback;
    solver->prsExportState = state;
    assert(solver->exportedClause != NULL);
}

void kissat_set_prs_import_clause_function(kissat *solver, 
    prs_import_clause_callback callback, void *state) {
    assert(callback != NULL);
    solver->prsImportClause = callback;
    solver->prsImportState = state;
    assert(solver->importedClause != NULL);
}

void kissat_set_prs_decide_function(kissat *solver, 
    prs_decide_callback callback, void *state) {
    assert(callback != NULL);
    solver->prsDecide = callback;
    solver->prsDecideState = state;
}

void kissat_set_prs_best_phase(kissat *solver, int* best_phase) {
    solver->prs_best_phase = best_phase;
}