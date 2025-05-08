#ifndef _KISSAT_WRAPPER_HPP_
#define _KISSAT_WRAPPER_HPP_

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
#include <functional>

#include "preprocess/preprocess.hpp"
#include "prs/clause.hpp"
#include "utils/concurrentqueue.h"
#include "prs/statistics.hpp"

extern "C" {
    #include "kissat.h"
}

// KissatSolver 作为包装类名称,提供更友好的C++接口
class KissatSolver {
public:
    KissatSolver(int id) : id(id) {
        solver = kissat_init();
        kissat_set_prs_export_clause_function(solver, static_export_callback, this);
        kissat_set_prs_import_clause_function(solver, static_import_callback, this);
    }
    ~KissatSolver() {
        if (solver) {
            kissat_release(solver);
            solver = nullptr;
        }
    }

    void configure(const char* name, int val) {
        kissat_set_option(solver, name, val);
    }

    void setBestPhase(int* best_phase) {
        kissat_set_prs_best_phase(solver, best_phase);
    }

    void read_from_proprocess(preprocess* pre) {
        kissat_reserve(solver, pre->vars);
        for (int i = 1; i <= pre->clauses; i++) {
            int l = pre->clause[i].size();
            for (int j = 0; j < l; j++)
                kissat_add(solver, pre->clause[i][j]);
            kissat_add(solver, 0);
        }
    }

    void read_from_vector(std::vector<std::vector<int>>& clauses, int vars) {
        kissat_reserve(solver, vars);
        for (int i = 0; i < clauses.size(); i++) {
            int l = clauses[i].size();
            for (int j = 0; j < l; j++)
                kissat_add(solver, clauses[i][j]);
            kissat_add(solver, 0);
        }
    }

    void add(int lit) {
        kissat_add(solver, lit);
    }

    int solve() {
        return kissat_solve(solver);
    }

    int getValue(int var) {
        int v = abs(var);
        int tmp = kissat_value(solver, v);
        if (!tmp) tmp = v;
        if (var < 0) tmp = -tmp;
        return tmp;
    }

    void terminate() {
        kissat_terminate(solver);
    }

    void reserve(int vars) {
        kissat_reserve(solver, vars);
    }

    // 添加回调函数类型定义
    using ExportCallback = std::function<void(const int id, std::vector<std::shared_ptr<Clause>>&)>;
    
    // 设置回调函数的方法
    void setExportCallback(ExportCallback callback) {
        export_callback = callback;
    }

    void importClause(std::shared_ptr<Clause> clause) {
        import_clause_queue.enqueue(clause);
    }

    void broadenExportLimit() {
        good_lbd++;
    }

    void restrictExportLimit() {
        if(good_lbd > 2) good_lbd--;
    }


    void setSBVA(bool sbva) {
        this->sbva = sbva;
    }

    bool getSBVA() {
        return sbva;
    }

private:

    static void static_export_callback(void* state, cvec* clause, int lbd) {
        KissatSolver* self = static_cast<KissatSolver*>(state);
        self->my_export_callback(clause, lbd);
    }

    void my_export_callback(cvec* clause, int lbd) {
        assert(clause->sz > 0);
        
        if(lbd > good_lbd) return;

        std::shared_ptr<Clause> clause_ptr = std::make_shared<Clause>(lbd, clause->sz);
        for (size_t i = 0; i < clause->sz; ++i) {
            clause_ptr->literals[i] = clause->data[i];
        }
        
        learned_clause.push_back(clause_ptr);

        assert(export_callback);
        export_callback(id, learned_clause);
    }

    static int static_import_callback(void* state, cvec* clause, int *lbd) {
        KissatSolver* self = static_cast<KissatSolver*>(state);
        return self->my_import_callback(clause, lbd);
    }

    int my_import_callback(cvec* clause, int *lbd) {
        assert(clause->sz == 0);
        // printf("thread %d import clause\n", id);
        std::shared_ptr<Clause> clause_ptr;
        if(!import_clause_queue.try_dequeue(clause_ptr)) {
            return -1;
        }
        // 将clause_ptr中的数据复制到clause中
        for(int i = 0; i < clause_ptr->size; i++) {
            cvec_push(clause, clause_ptr->literals[i]);
        }
        *lbd = clause_ptr->lbd;
        return 1;
    }

    moodycamel::ConcurrentQueue<std::shared_ptr<Clause>> import_clause_queue;
    std::vector<std::shared_ptr<Clause>> learned_clause;

    kissat* solver;

    const int id;

    int good_lbd = 2;

    ExportCallback export_callback;  // 添加回调函数成员变量

    // Statistics *statistics;


    bool sbva = false;
};

#endif
