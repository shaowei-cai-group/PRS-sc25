#include "../../preprocess/preprocess.hpp"
#include "../parellel_pre.hpp"
#include "../prs.hpp"
#include <chrono>
#include <thread>

// 解决SAT问题
int PRS::mix_solve(const char* filename) {

    int nbPrsKissat = 20;
    int nbSbvaKissat = 10;

    int nbPrsYalsat = 1;
    int nbSbvaYalsat = 1;

    int nbKissat = nbPrsKissat + nbSbvaKissat;
    int nbYalsat = nbPrsYalsat + nbSbvaYalsat;

    printf("c read and proprocessing PRS...\n");

    ParallelPreprocess pp;
    int res = pp.do_serial_preprocess(filename);
    if (res == 20) return 20; // UNSAT
    else if (res == 10) { // SAT
        for (int i = 1; i <= pp.get_preprocess()->vars; i++) {
            model.push(pp.get_preprocess()->model[i]);
        }
        return 10;
    }

    if (pp.get_preprocess()->clauses > 33554431) {
        printf("c too many clauses, yalsat not used\n");
        nbPrsYalsat = 0;
        nbSbvaYalsat = 0;
        nbPrsKissat += 1;
        nbSbvaKissat += 1;
    }

    printf("c create solver instances ...\n");

    // 初始化last_share_times
    last_share_times.resize(OPT(threads));    
    // 初始化桶排序
    buckets.resize(OPT(threads));
    
    // 清空原有求解器列表（如果有）
    for (auto solver : solvers) {
        if (solver) delete solver;
    }
    solvers.clear();
    
    for (auto solver : yalsat_solvers) {
        if (solver) delete solver;
    }
    yalsat_solvers.clear();

    // 创建并初始化Kissat求解器实例
    printf("c creating %d Kissat solver instances (%d PRS + %d SBVA) ...\n", nbKissat, nbPrsKissat, nbSbvaKissat);
    for (int i = 0; i < nbKissat; i++) {
        solvers.push_back(new KissatSolver(i));
        solvers[i]->setExportCallback([this](int id, std::vector<std::shared_ptr<Clause>>& clauses) {
            this->export_callback(id, clauses);
        });
    }

    // 创建并初始化Yalsat求解器实例
    printf("c creating %d Yalsat solver instances (%d PRS + %d SBVA) ...\n", nbYalsat, nbPrsYalsat, nbSbvaYalsat);
    for (int i = 0; i < nbYalsat; i++) {
        yalsat_solvers.push_back(new YalsatSolver(i));
    }
    
    // 配置所有求解器的基本参数
    printf("c configuring all solvers...\n");
    configure_solvers();

    printf("c prs kissat read instance ...\n");

    // Kissat求解器读取预处理后的实例
    for (int i = 0; i < nbPrsKissat; i++) {
        read_futures.push_back(std::async(std::launch::async, [this, i, &pp]() {
            solvers[i]->read_from_proprocess(pp.get_preprocess());
            return 0;
        }));
    }
    // 等待所有PRS-Kissat读取完成
    for (int i = 0; i < nbPrsKissat; i++) {
        read_futures[i].wait();
    }
    read_futures.clear();
    
    // Yalsat求解器读取预处理后的实例
    printf("c prs yalsat read instance ...\n");
    for (int i = 0; i < nbPrsYalsat; i++) {
        read_futures.push_back(std::async(std::launch::async, [this, i, &pp]() {
            yalsat_solvers[i]->read_from_proprocess(pp.get_preprocess());
            return 0;
        }));
    }
    // 等待所有PRS-Yalsat读取完成
    for (int i = 0; i < nbPrsYalsat; i++) {
        read_futures[i].wait();
    }
    read_futures.clear();

    printf("c start prs-yalsat(%d) and prs-kissat(%d) solving ...\n", nbPrsYalsat, nbPrsKissat);

    

    for (int i=0; i<nbPrsKissat; i++) {
        kissat_futures.push_back(std::async(std::launch::async, [this, i]() {
            return solvers[i]->solve();
        }));
    }    
    for (int i=0; i<nbPrsYalsat; i++) {
        yalsat_futures.push_back(std::async(std::launch::async, [this, i]() {
            return yalsat_solvers[i]->solve();
        }));
    }

    const int sbva_timeout = 500;

    printf("c start sbva simplification (timeout: %d) ...\n", sbva_timeout);

    // 已经在前面调用过configure_solvers，这里无需再次调用
    // configure_solvers();

    sbva_future = std::async(std::launch::async, [this, &pp, nbSbvaKissat, nbSbvaYalsat]() {
        return pp.do_sbva_preprocess(sbva_timeout, nbSbvaKissat + nbSbvaYalsat); // 原函数不接受超时参数
    });

    // 设置SBVA超时
    auto sbva_start_time = std::chrono::steady_clock::now();

    int completed_thread = -1;
    bool any_success = false;
    res = 0;
    bool sbva_completed = false;
    preprocess* pre = pp.get_preprocess();

    // 主循环，处理求解结果和SBVA化简完成后启动新求解器
    while(!any_success) {
        // 检查PRS-Kissat求解器结果
        for (int i = 0; i < nbPrsKissat && !any_success; i++) {
            if (kissat_futures[i].valid()) {
                auto status = kissat_futures[i].wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    res = kissat_futures[i].get();
                    if (res != 0) {
                        printf("c problem solved by PRS-Kissat thread %d\n", i);
                        completed_thread = i;
                        any_success = true;
                        
                        // 处理SAT结果
                        if (res == 10) {
                            model.clear();
                            for (int j = 1; j <= pre->vars; j++) {
                                model.push(solvers[i]->getValue(j));
                            }
                        }
                        break;
                    }
                }
            }
        }

        // 检查PRS-Yalsat求解器结果
        for (int i = 0; i < nbPrsYalsat && !any_success; i++) {
            if (yalsat_futures[i].valid()) {
                auto status = yalsat_futures[i].wait_for(std::chrono::milliseconds(100));
                if (status == std::future_status::ready) {
                    res = yalsat_futures[i].get();
                    if (res != 0) {
                        printf("c problem solved by PRS-Yalsat thread %d\n", i);
                        any_success = true;
                        
                        // 处理SAT结果 - Yalsat特殊处理
                        if (res == 10) {
                            model.clear();
                            for (int j = 1; j <= pre->vars; j++) {
                                model.push(yalsat_solvers[i]->getValue(j));
                            }
                        }
                        break;
                    }
                }
            }
        }

        // 检查SBVA是否完成或超时，如果完成则启动SBVA求解器
        if (!sbva_completed) {
            // 检查SBVA是否超时
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - sbva_start_time).count();
            
            // 检查SBVA是否正常完成
            auto sbva_status = sbva_future.wait_for(std::chrono::milliseconds(100));
            if (sbva_status == std::future_status::ready) {
                int sbva_res = sbva_future.get();
                sbva_completed = true;
                printf("c sbva simplification completed in %d seconds\n", (int)elapsed_seconds);
                
                // 如果 SBVA 没启动
                if (sbva_res == 0) {
                    // 启动SBVA-Kissat求解器
                    for (int i = nbPrsKissat; i < nbPrsKissat + nbSbvaKissat; i++) {
                        printf("c (sbva failed) starting normal-Kissat solver %d\n", i);
                        kissat_futures.push_back(std::async(std::launch::async, [this, i, pre]() {
                            solvers[i]->read_from_proprocess(pre);
                            return solvers[i]->solve();
                        }));
                    }
                    // 启动SBVA-Yalsat求解器
                    for (int i = nbPrsYalsat; i < nbPrsYalsat + nbSbvaYalsat; i++) {
                        printf("c (sbva failed) starting normal-Yalsat solver %d\n", i);
                        yalsat_futures.push_back(std::async(std::launch::async, [this, i, pre]() {
                            yalsat_solvers[i]->read_from_proprocess(pre);
                            return yalsat_solvers[i]->solve();
                        }));
                    }
                } else {
                    // 启动SBVA-Kissat求解器
                    for (int i = nbPrsKissat; i < nbPrsKissat + nbSbvaKissat; i++) {
                        printf("c starting SBVA-Kissat solver %d\n", i);
                        kissat_futures.push_back(std::async(std::launch::async, [this, i, &pp]() {
                            solvers[i]->read_from_vector(pp.clauses, pp.varCount);
                            return solvers[i]->solve();
                        }));
                    }
                    
                    // 启动SBVA-Yalsat求解器
                    for (int i = nbPrsYalsat; i < nbPrsYalsat + nbSbvaYalsat; i++) {
                        printf("c starting SBVA-Yalsat solver %d\n", i);
                        yalsat_futures.push_back(std::async(std::launch::async, [this, i, &pp]() {
                            yalsat_solvers[i]->read_from_vector(pp.clauses, pp.varCount);
                            return yalsat_solvers[i]->solve();
                        }));
                    }
                }
            }
        }
        
        // 如果SBVA已完成，检查SBVA求解器的结果
        if (sbva_completed && !any_success) {
            // 检查SBVA-Kissat求解器结果
            for (int i = nbPrsKissat; i < nbPrsKissat + nbSbvaKissat && !any_success; i++) {
                // SBVA-Kissat在kissat_futures中的索引
                int idx = i;
                if (idx < kissat_futures.size() && kissat_futures[idx].valid()) {
                    auto status = kissat_futures[idx].wait_for(std::chrono::milliseconds(100));
                    if (status == std::future_status::ready) {
                        res = kissat_futures[idx].get();
                        if (res != 0) {
                            printf("c problem solved by SBVA-Kissat thread %d\n", i);
                            completed_thread = i;
                            any_success = true;
                            
                            // 处理SAT结果
                            if (res == 10) {
                                model.clear();
                                for (int j = 1; j <= pre->vars; j++) {
                                    model.push(solvers[i]->getValue(j));
                                }
                            }
                            break;
                        }
                    }
                }
            }
            
            // 检查SBVA-Yalsat求解器结果
            for (int i = nbPrsYalsat; i < nbPrsYalsat + nbSbvaYalsat && !any_success; i++) {
                // SBVA-Yalsat在yalsat_futures中的索引
                int idx = i;
                if (idx < yalsat_futures.size() && yalsat_futures[idx].valid()) {
                    auto status = yalsat_futures[idx].wait_for(std::chrono::milliseconds(100));
                    if (status == std::future_status::ready) {
                        res = yalsat_futures[idx].get();
                        if (res != 0) {
                            printf("c problem solved by SBVA-Yalsat thread %d\n", i);
                            any_success = true;
                            
                            // 处理SAT结果 - Yalsat特殊处理
                            if (res == 10) {
                                model.clear();
                                for (int j = 1; j <= pre->vars; j++) {
                                    model.push(yalsat_solvers[i]->getValue(j));
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
        
        // 短暂睡眠，避免CPU过度使用
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 终止所有求解器
    printf("c terminating all solvers...\n");
    
    // 终止Kissat求解器
    for (int i = 0; i < nbKissat; i++) {
        if (solvers.size() > i && solvers[i] != nullptr) {
            solvers[i]->terminate();
        }
    }
    
    // 终止Yalsat求解器
    for (int i = 0; i < nbYalsat; i++) {
        if (yalsat_solvers.size() > i && yalsat_solvers[i] != nullptr) {
            yalsat_solvers[i]->terminate();
        }
    }
    
    // 等待所有任务完成
    for (auto& future : kissat_futures) {
        if (future.valid()) {
            try {
                // 设置短暂的超时，避免永久等待
                future.wait_for(std::chrono::seconds(1));
            } catch (...) {
                // 忽略异常
            }
        }
    }

    // printf("kill yalsat\n");
    
    for (auto& future : yalsat_futures) {
        if (future.valid()) {
            try {
                // 设置短暂的超时，避免永久等待
                future.wait_for(std::chrono::seconds(1));
            } catch (...) {
                // 忽略异常
            }
        }
    }

    // printf("kill done\n");

    // 处理SAT结果，映射到原始变量
    if (res == 10) {
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
    // printf("c cleaning up solver resources...\n");
    
    // for (auto solver : solvers) {
    //     if (solver) delete solver;
    // }
    // solvers.clear();
    
    // for (auto solver : yalsat_solvers) {
    //     if (solver) delete solver;
    // }
    // yalsat_solvers.clear();

    return res;
}