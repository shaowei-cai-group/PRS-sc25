#ifndef YALSAT_HPP
#define YALSAT_HPP

#include <memory>
#include <string>
#include <vector>
#include <functional>

extern "C" {
    #include "yals.h"
}

class YalsatSolver {
private:
    Yals* solver;
    bool should_terminate;
    int orivars;
    // 存储最佳相位的成员变量


public:
    int id;
    std::vector<int> best_phase;
    int min_unsat = -1;

    YalsatSolver(int id) : solver(nullptr), should_terminate(false), id(id) {
        solver = yals_new();
    }

    ~YalsatSolver() {
        if (solver) {
            yals_del(solver);
            solver = nullptr;
        }
    }

    void read_from_proprocess(preprocess* pre) {
        for (int i = 1; i <= pre->clauses; i++) {
            int l = pre->clause[i].size();
            for (int j = 0; j < l; j++)
                yals_add(solver, pre->clause[i][j]);
            yals_add(solver, 0);
        }
        orivars = pre->orivars;
    }

    void read_from_vector(std::vector<std::vector<int>>& clauses, int vars) {
        for (int i = 0; i < clauses.size(); i++) {
            int l = clauses[i].size();
            for (int j = 0; j < l; j++)
                yals_add(solver, clauses[i][j]);
            yals_add(solver, 0);
        }
        orivars = vars;
    }

    // 添加一个文字
    void add(int lit) {
        if (solver) {
            yals_add(solver, lit);
        }
    }

    // 设置随机种子
    void setRandomSeed(unsigned long long seed) {
        if (solver) {
            yals_srand(solver, seed);
        }
    }

    // 设置翻转次数上限
    void setFlipsLimit(long long limit) {
        if (solver) {
            yals_setflipslimit(solver, limit);
        }
    }

    // 设置内存使用上限
    void setMemsLimit(long long limit) {
        if (solver) {
            yals_setmemslimit(solver, limit);
        }
    }

    // 设置选项
    int setOption(const std::string& name, int value) {
        if (solver) {
            return yals_setopt(solver, name.c_str(), value);
        }
        return 0;
    }

    // 检查并更新最佳相位，如果有回调则调用
    void checkAndUpdateBestPhase() {
        if (!solver) return;
        
        // 获取当前最佳相位
        min_unsat = yals_minimum(solver);

        // printf("c min_unsat: %d\n", min_unsat);

        if(min_unsat != 2147483647) {
            best_phase.resize(orivars + 1);
            for (int i = 1; i <= orivars; i++) {
                best_phase[i] = yals_deref(solver, i) > 0 ? 1 : -1;
                assert(best_phase[i] == 1 || best_phase[i] == -1);
            }
        } else {
            best_phase.clear();
        }
    }

    // 求解SAT问题
    int solve() {
        if (solver) {
            // 设置终止回调
            yals_seterm(solver, [](void* ptr) -> int {
                YalsatSolver* self = static_cast<YalsatSolver*>(ptr);
                return self->should_terminate ? 1 : 0;
            }, this);
            
            int result = yals_sat(solver);
            return result;  // 10表示SAT
        }
        return 0;
    }

    // 终止求解
    void terminate() {
        should_terminate = true;
    }

    // 获取解
    bool getValue(int lit) {
        if (solver) {
            return yals_deref(solver, lit) > 0;
        }
        return false;
    }

    // 获取翻转次数
    long long getFlips() {
        if (solver) {
            return yals_flips(solver);
        }
        return 0;
    }

    // 获取内存操作次数
    long long getMems() {
        if (solver) {
            return yals_mems(solver);
        }
        return 0;
    }

    // status
    void printStatus() {
        if (solver) {
            yals_stats(solver);
        }
    }

};

#endif // YALSAT_HPP
