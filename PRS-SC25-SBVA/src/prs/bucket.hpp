#pragma once

#include <vector>
#include <memory>
#include "clause.hpp"
#include "options.hpp"

// 桶排序类，用于管理共享子句
class Bucket {
public:
    // 初始化桶结构
    Bucket() = default;

    // 添加一个子句到合适的桶中
    bool addClause(std::shared_ptr<Clause> clause) {
        int sz = clause->size;
        while (sz > buckets.size()) {
            buckets.emplace_back();
        }
        
        if (sz * (buckets[sz - 1].size() + 1) <= OPT(share_lits)) {
            buckets[sz - 1].push_back(clause);
            return true;
        }
        return false;
    }
    
    // 收集适合分享的子句
    std::vector<std::shared_ptr<Clause>> collectSharingClauses() {
        std::vector<std::shared_ptr<Clause>> share_buffer;
        
        int space = OPT(share_lits);
        for (int i = 0; i < buckets.size(); i++) {
            int clause_num = space / (i + 1);
            if (clause_num == 0) break;
            
            if (clause_num > buckets[i].size()) {
                space -= buckets[i].size() * (i + 1);
                for (auto& clause : buckets[i]) {
                    share_buffer.push_back(clause);
                }
                buckets[i].clear();
            } else {
                space -= clause_num * (i + 1);
                for (int j = 0; j < clause_num; j++) {
                    share_buffer.push_back(buckets[i].back());
                    buckets[i].pop_back();
                }
            }
        }
        
        // 返回剩余空间百分比
        share_percent = (OPT(share_lits) - space) * 100 / OPT(share_lits);
        
        return share_buffer;
    }
    
    // 获取最近一次分享的填充百分比
    int getSharePercent() const {
        return share_percent;
    }
    
    // 清空所有桶
    void clear() {
        for (auto& bucket : buckets) {
            bucket.clear();
        }
    }
    
private:
    // 按子句长度组织的桶
    std::vector<std::vector<std::shared_ptr<Clause>>> buckets;
    
    // 上次分享的填充百分比
    int share_percent = 0;
}; 