#include "prs.hpp"
#include "../preprocess/preprocess.hpp"
#include "parellel_pre.hpp"

// 构造函数
PRS::PRS() {
    
}

// 析构函数
PRS::~PRS() {
    
}

// 配置求解器参数
void PRS::configure_solvers() {

    int num_solvers = solvers.size();

    int num_sat_group = 0;
    int num_unsat_group = 0;
    int num_default_group = 0;

    // 配置所有求解器的基本参数
    for (int i = 0; i < num_solvers; i++) {
        solvers[i]->configure("seed", rand());
        solvers[i]->configure("threads", num_solvers);
        solvers[i]->configure("quiet", 1);
        solvers[i]->configure("check", 0);
        solvers[i]->configure("factor", 0);
        // 随机化求解顺序
        solvers[i]->configure("order_reset", i);
        // 部分初始化为全假
        solvers[i]->configure("phase", i % 2);

        // 随机化是否使用SBVA
        solvers[i]->setSBVA(rand() % 2);
    }


    for(int i=0; i<num_solvers; i++) {
        int group_rand = rand() % 3;
        if (group_rand == 0) {
            num_sat_group++;
            // 配置SAT倾向组
            // 禁用sweep
            solvers[i]->configure("sweep", 0);
            solvers[i]->configure("target", 2);

            // 减少重启
            solvers[i]->configure("restartint", 50 + rand() % 100);
            solvers[i]->configure("restartmargin", rand() % 25 + 10);

            // 增加稳定性
            solvers[i]->configure("stable", 2);

            // 增加漫步
            solvers[i]->configure("walkinitially", 1);
            solvers[i]->configure("walkrounds", rand() % (1 << 4));

            // 减少层级
            solvers[i]->configure("tier1", 2);
            solvers[i]->configure("tier2", 3);
        } else if (group_rand == 1) {
            num_unsat_group++;
            // 配置UNSAT倾向组
            // 启用并增强sweep
            solvers[i]->configure("sweep", 1);
            solvers[i]->configure("sweepclauses", 1024 + rand() % 1024);
            solvers[i]->configure("sweepcomplete", 0);
            solvers[i]->configure("sweepdepth", 2 + rand() % 2);
            solvers[i]->configure("sweepeffort", 100 + rand() % 100);
            solvers[i]->configure("sweepfliprounds", 1);
            solvers[i]->configure("sweepmaxclauses", 32768 + rand() % 32768);
            solvers[i]->configure("sweepmaxdepth", 3 + rand() % 2);
            solvers[i]->configure("sweepmaxvars", 8192 + rand() % 8192);
            solvers[i]->configure("sweeprand", 1);
            solvers[i]->configure("sweepvars", 256 + rand() % 256);

            // 增加重启减少稳定性
            solvers[i]->configure("stable", 0);
            solvers[i]->configure("restartmargin", 10 + (rand() % 5));
        } else {
            num_default_group++;
            // 配置默认组
            solvers[i]->configure("sweep", 1);
        }
    }

    printf("c num_solvers: %d, num_sat_group: %d, num_unsat_group: %d, num_default_group: %d\n", 
            num_solvers, num_sat_group, num_unsat_group, num_default_group);
}

// 导出回调函数
void PRS::export_callback(const int id, std::vector<std::shared_ptr<Clause>>& clauses) {
    // 获取当前时间
    auto current_time = std::chrono::steady_clock::now();
    
    // 检查是否达到分享时间间隔
    if (last_share_times[id].time_since_epoch().count() == 0 || 
        std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - last_share_times[id]).count() >= OPT(share_intv)) {
        
        // 更新最后分享时间
        last_share_times[id] = current_time;

        // 共享子句并根据填充率调整导出限制
        
        Bucket& bucket = buckets[id];

        // 将子句添加到桶中
        while (!clauses.empty()) {
            auto clause = clauses.back();
            clauses.pop_back();
            bucket.addClause(clause);
        }

        // 收集要分享的子句
        std::vector<std::shared_ptr<Clause>> share_buffer = bucket.collectSharingClauses();

        // 向其他求解器分享子句
        for (int i = 0; i < solvers.size(); i++) {
            if (i != id) {
                for (auto& clause : share_buffer) {
                    solvers[i]->importClause(clause);
                }
            }
        }

        // 更新桶的填充率
        int percent = bucket.getSharePercent();
        // 负反馈调节
        if (percent < 75) solvers[id]->broadenExportLimit();
        if (percent > 98) solvers[id]->restrictExportLimit();
    }
}

// 获取求解结果模型
vec<int>& PRS::getModel() {
    return model;
} 