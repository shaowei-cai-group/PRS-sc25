#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <future>

#include "solvers/kissat.hpp"
#include "solvers/yalsat.hpp"
#include "deversity.hpp"
#include "options.hpp"
#include "utils/vec.hpp"
#include "prs/bucket.hpp"

// 前向声明
class preprocess;

// 并行SAT求解器类
class PRS {
public:
    // 构造函数，初始化并行SAT求解器环境
    PRS();
    
    // 析构函数
    ~PRS();
    
    // 配置求解器参数
    void configure_solvers();
    
    // 解决SAT问题
    int solve(const char* filename);
    int mix_solve(const char* filename);
    
    // 获取求解结果模型
    vec<int>& getModel();

private:
    // 子句共享处理函数
    int shareClauses(std::vector<std::shared_ptr<Clause>>& clauses, int id);
    
    // 导出回调函数
    void export_callback(const int id, std::vector<std::shared_ptr<Clause>>& clauses);
    
    // 求解器实例列表
    std::vector<KissatSolver*> solvers;
    std::vector<YalsatSolver*> yalsat_solvers;
    
    // 每个线程对应的桶结构
    std::vector<Bucket> buckets;
    
    // 结果模型
    vec<int> model;
    
    // 记录每个线程上次分享时间
    std::vector<std::chrono::steady_clock::time_point> last_share_times;


    // thread_local
    std::vector<std::future<int>> read_futures;
    std::vector<std::future<int>> kissat_futures;
    std::vector<std::future<int>> yalsat_futures;
    std::future<int> sbva_future;


    

}; 