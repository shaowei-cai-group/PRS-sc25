#include "../prs.hpp"
#include "../parellel_pre.hpp"

// 解决SAT问题
int PRS::solve(const char* filename) {
    int res = 0;

    printf("c read and preprocessing...\n");
    
    printf("c solving...\n");

    ParallelPreprocess pp;
    res = pp.perform_preprocess(filename);

    printf("c preprocessing done\n");
    
    if (res == 20) return 20; // UNSAT
    else if (res == 10) { // SAT
        for (int i = 1; i <= pp.get_preprocess()->vars; i++) {
            model.push(pp.get_preprocess()->model[i]);
        }
        return 10;
    }

    // 初始化last_share_times
    last_share_times.resize(OPT(threads));
    
    // 初始化桶排序
    buckets.resize(OPT(threads));

    // 创建并初始化求解器实例
    for (int i = 0; i < OPT(threads); i++) {
        solvers.push_back(new KissatSolver(i));
        solvers[i]->setExportCallback([this](const int id, std::vector<std::shared_ptr<Clause>>& clauses) {
            this->export_callback(id, clauses);
        });
    }

    if(OPT(yalsat)) {
        solvers[0]->setBestPhase(nullptr);
        for (int i=1; i<OPT(threads); i++) {
            std::vector<int>& best_phase = pp.get_yalsat_solvers()[i-1]->best_phase;
            if(best_phase.size() == pp.get_preprocess()->orivars + 1) {
                solvers[i]->setBestPhase(best_phase.data());
                printf("c set best phase for solver %d, min_unsat: %d\n", i, pp.get_yalsat_solvers()[i-1]->min_unsat);
            } else {
                solvers[i]->setBestPhase(nullptr);
                printf("c no best phase for solver %d\n", i);
            }
        }
    } else {
        printf("c disable yalsat best phase because of too many clauses\n");
        for (int i=0; i<OPT(threads); i++) {
            solvers[i]->setBestPhase(nullptr);
        }
    }

    // 配置求解器
    configure_solvers();

    std::vector<std::future<int>> futures;

    preprocess* pre = pp.get_preprocess();
    
    // 并行启动所有求解器
    for (int i = 0; i < OPT(threads); i++) {
        futures.push_back(std::async(std::launch::async, [this, pre, i]() {
            solvers[i]->read_from_proprocess(pre);
            int result = solvers[i]->solve();
            return result;
        }));
    }

    int completed_thread = -1;
    bool any_success = false;
    
    // 轮询所有future，检查哪个线程先完成
    while (!any_success) {
        // 检查kissat线程
        for (int i = 0; i < OPT(threads); i++) {
            if (futures[i].valid()) {
                auto status = futures[i].wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    res = futures[i].get();
                    if (res != 0) {
                        completed_thread = i;
                        any_success = true;
                        break;
                    }
                }
            }
        }
    }
    
    // 终止其他求解器
    for (int i = 0; i < OPT(threads); i++) {
        if (i != completed_thread) {
            solvers[i]->terminate();
        }
    }
    
    // 等待所有线程结束
    for (int i = 0; i < futures.size(); i++) {
        if (i != completed_thread && futures[i].valid()) {
            futures[i].wait();
        }
    }

    printf("c problem solved by thread %d\n",  completed_thread);

    // 处理SAT结果
    if (res == 10) {
        // 从成功求解器获取模型
        model.clear();
            // 从kissat获取模型
        for (int i = 1; i <= pre->vars; i++) {
            model.push(solvers[completed_thread]->getValue(i));
        }
        
        // 映射到原始变量
        for (int i = 1; i <= pre->orivars; i++)
            if (pre->mapto[i]) pre->mapval[i] = (model[abs(pre->mapto[i])-1] > 0 ? 1 : -1) * (pre->mapto[i] > 0 ? 1 : -1);
        
        pre->get_complete_model();
        model.clear();
        for (int i = 1; i <= pre->orivars; i++) {
            model.push(i * pre->mapval[i]);
        }
    }

    // 释放求解器资源
    for (auto solver : solvers) {
        delete solver;
    }

    return res;
}