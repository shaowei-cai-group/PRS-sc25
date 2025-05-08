#include "preprocess/preprocess.hpp"
#include "options.hpp"
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <memory>
#include <chrono>
#include <future>
#include "solvers/yalsat.hpp"
#include "preprocess/sbva/StructuredBva.hpp"

class ParallelPreprocess {
private:
    preprocess* pre;
    // 用于线程间共享状态
    std::mutex model_mutex;
    std::atomic<bool> preprocess_completed;
    std::vector<std::unique_ptr<YalsatSolver>> yalsat_solvers;
    // random
    std::mt19937 engine{std::random_device{}()};
    std::uniform_int_distribution<int> uniform{1, 100};
    
public:

    std::vector<std::vector<int>> clauses;
    unsigned varCount = 0;

    ParallelPreprocess() {
        pre = new preprocess();
        preprocess_completed.store(false);
    }
    ~ParallelPreprocess() {
        delete pre;
    }

    preprocess* get_preprocess() {
        return pre;
    }

    std::vector<std::unique_ptr<YalsatSolver>>& get_yalsat_solvers() {
        return yalsat_solvers;
    }
    
    int perform_preprocess(const char* filename) {
        pre->read_file(filename);
        if(OPT(yalsat) && pre->clauses > 33554431) {
            printf("c yalsat cannot handle more than 33554431 clauses\n");
            OPT(yalsat) = 0;
        }

        if(OPT(yalsat) && OPT(mode) == 0) {
            printf("c start local search init phase\n");
            return do_parallel_preprocess(filename);
        } else {
            printf("c start serial preprocess\n");
            return do_serial_preprocess(filename);
        }
    }

    int do_sbva_preprocess(int timeout, int num_sbva_threads) {
        if(pre->clauses < 1e8) {
            //copy clauses
            for(int i=1; i<pre->clause.size(); i++) {
                clauses.push_back(std::vector<int>());
                for(int j=0; j<pre->clause[i].size(); j++) {
                    clauses.back().push_back(pre->clause[i][j]);
                }
            }

            // 创建和运行SBVA线程
            std::vector<std::shared_ptr<StructuredBVA>> sbva_instances;
            std::vector<std::thread> sbva_threads;
            std::atomic<bool> timeout_reached(false);

            for(int i=0; i<num_sbva_threads; i++) {
                auto sbva = std::make_shared<StructuredBVA>(num_sbva_threads + i);
                sbva->diversify(engine, uniform);
                sbva->printParameters();
                sbva_instances.push_back(sbva);
            }
            
            // 创建多个SBVA实例和线程
            for(int i=0; i<num_sbva_threads; i++) {
                sbva_threads.push_back(std::thread([this, i, &sbva_instances, num_sbva_threads]() {
                    auto sbva = sbva_instances[i];
                    sbva->addInitialClauses(clauses, pre->vars);
                    sbva->run();
                    sbva->printStatistics();
                    if(sbva->isInitialized() && sbva->getClausesCount() > 0) {
                        printf("c sbva %d done nbvars %d -> %d, nbClauses: %d -> %d, deleted: %d\n", i,
                              sbva->getVariablesCount(), sbva->getVariablesCount(), 
                              sbva->getClausesCount() + sbva->getNbClausesDeleted(), sbva->getClausesCount(),
                              sbva->getNbClausesDeleted());
                    }
                }));
            }
            
            // 设置超时
            auto timeout_t = std::chrono::seconds(timeout);
            std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
            
            // 等待线程完成或超时
            std::thread timeout_thread([&]() {
                while(!timeout_reached.load() && 
                      std::chrono::system_clock::now() - start < timeout_t) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                
                // 如果超时且还有线程在运行，中断它们
                if(std::chrono::system_clock::now() - start >= timeout_t) {
                    timeout_reached.store(true);
                    printf("c sbva timeout after %d seconds, interrupting\n", timeout);
                    
                    for(auto& sbva : sbva_instances) {
                        sbva->setInterrupt();
                    }
                }
            });
            
            // 等待所有线程完成
            for(auto& t : sbva_threads) {
                if(t.joinable()) {
                    t.join();
                }
            }
            
            // 停止超时线程
            timeout_reached.store(true);
            if(timeout_thread.joinable()) {
                timeout_thread.join();
            }
            
            // 选择最好的SBVA结果
            if(!sbva_instances.empty()) {
                auto best_sbva_it = std::max_element(
                    sbva_instances.begin(), 
                    sbva_instances.end(), 
                    [](const std::shared_ptr<StructuredBVA>& a, const std::shared_ptr<StructuredBVA>& b) {
                        return a->getNbClausesDeleted() < b->getNbClausesDeleted();
                    }
                );
                
                if(best_sbva_it != sbva_instances.end()) {
                    auto best_sbva = *best_sbva_it;
                    if(best_sbva->isInitialized() && best_sbva->getClausesCount() > 0) {
                        printf("c best sbva done nbvars %d -> %d, nbClauses: %d -> %d, deleted: %d\n", 
                               pre->vars, best_sbva->getVariablesCount(), 
                               pre->clauses, best_sbva->getClausesCount(),
                               best_sbva->getNbClausesDeleted());
                        clauses = std::move(best_sbva->getClauses());
                        varCount = best_sbva->getVariablesCount();
                        return 1;
                    }
                }
            }
            
            printf("c no effective sbva found\n");
            varCount = pre->vars;
        } else {
            printf("c sbva not used\n");
            varCount = pre->vars;
        }
        return 0;
    }

    int do_serial_preprocess(const char* filename) {
        pre->read_file(filename);
        int preprocess_result = pre->do_preprocess();
        // if(preprocess_result == 0) {
        //     // copy clauses
        //     for (int i = 1; i < pre->clause.size(); i++) {
        //         clauses.push_back(std::vector<int>());
        //         for (int j = 0; j < pre->clause[i].size(); j++) {
        //             clauses.back().push_back(pre->clause[i][j]);
        //         }
        //     }

        //     StructuredBVA sbva(-1);
        //     sbva.setTieBreakHeuristic(SBVATieBreak::MOSTOCCUR);
        //     sbva.printParameters();
        //     sbva.addInitialClauses(clauses, pre->vars);
        //     sbva.run();

        //     sbva.printStatistics();

        //     if (sbva.isInitialized())
        //     {
        //         printf("c fuck sbva done nbvars %d -> %d, nbClauses: %d -> %d\n", pre->vars, sbva.getVariablesCount(), pre->clauses, sbva.getClausesCount());
        //         clauses = std::move(sbva.getClauses());
        //         varCount = sbva.getVariablesCount();
        //         sbva.clearAll();
        //         exit(0);
        //     }
        // }

        return preprocess_result;
    }

    int do_parallel_preprocess(const char* filename) {        
        // 设置线程数目
        int num_threads = OPT(threads);
        if (num_threads <= 1) {
            num_threads = 2; // 至少使用两个线程
        }

        // create yalsat solvers
        for (int i = 0; i < num_threads - 1; i++) {
            auto solver = std::make_unique<YalsatSolver>(i);
            yalsat_solvers.push_back(std::move(solver));
        }

        // parallel read cnf for yalsat
        std::vector<std::thread> yalsat_read_threads;
        for (int i = 0; i < num_threads - 1; i++) {
            yalsat_read_threads.push_back(std::thread([this, i]() {
                yalsat_solvers[i]->read_from_proprocess(pre);
            }));
        }

        for (auto& t : yalsat_read_threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        preprocess_completed.store(false);
        
        // 预处理结果
        int preprocess_result = 0;

        // 启动一个线程进行经典化简
        std::thread preprocess_thread([this, &preprocess_result]() {
            preprocess_result = pre->do_preprocess();
            // 预处理完成，标记状态
            std::lock_guard<std::mutex> lock(model_mutex);
            preprocess_completed.store(true);
        });
        
        // 启动OPT(threads)-1个线程运行yalsat，使用不同的参数
        std::vector<std::thread> yalsat_threads;
        for (int i = 0; i < num_threads - 1; i++) {
            yalsat_threads.push_back(std::thread([this, i]() {
                auto solver = yalsat_solvers[i].get();
                solver->setRandomSeed(i * 1000 + std::random_device{}());
                solver->solve();
            }));
        }
        
        // 等待预处理线程完成
        preprocess_thread.join();

        // 终止所有yalsat求解器
        for (auto& solver : yalsat_solvers) {
            if (solver) {
                solver->terminate();
            }
        }
        
        // 预处理线程已完成，等待所有yalsat线程结束
        for (auto& t : yalsat_threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        for (auto& solver : yalsat_solvers) {
            solver->checkAndUpdateBestPhase();
        }
        
        // 返回预处理的结果
        return preprocess_result;
    }
};