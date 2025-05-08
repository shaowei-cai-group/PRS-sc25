#pragma once

#include <queue>
#include <vector>
#include <cstdint>
#include <cmath>

class Statistics {
private:
    // 变量的最大编号
    int max_var;
    // 滑动窗口的大小
    int window_size;
    
    // 记录每个变量的决策次数
    std::vector<int> decide_counts;
    // 记录每个变量的正相位次数
    std::vector<int> positive_counts;
    
    // 滑动窗口队列，存储历史决策文字
    std::queue<int> history_lits;
    
    // 从文字获取变量编号（去除符号）
    inline int var(int lit) const {
        return std::abs(lit);
    }
    
    // 检查文字是否为正相位
    inline bool is_positive(int lit) const {
        return lit > 0;
    }

    bool initialized = false;
    
public:

    static double getDiversityScore(Statistics* sta, Statistics* stb) {
        assert(sta->max_var == stb->max_var);
        assert(sta->window_size == stb->window_size);

        double frequency_diversity = getVariableFrequencyDiversityScore(sta, stb);
        double phase_diversity = getPhaseAverageDiversityScore(sta, stb);
        double diversity = (frequency_diversity + phase_diversity) / 2;

        // printf("diversity: %f frequency_diversity: %f phase_diversity: %f \n", diversity, frequency_diversity, phase_diversity);

        return diversity;
    }

    static double getVariableFrequencyDiversityScore(Statistics* sta, Statistics* stb) {
        assert(sta->max_var == stb->max_var);
        assert(sta->window_size == stb->window_size);
        double diversity = 0.0;
        
        // 计算差异
        for (int i = 1; i <= sta->max_var; i++) {
            diversity += std::abs(sta->get_decide_frequency(i) - stb->get_decide_frequency(i));
        }
        
        // 使用变量数量归一化，计算平均差异
        return diversity / sta->max_var;
    }

    static double getPhaseAverageDiversityScore(Statistics* sta, Statistics* stb) {
        assert(sta->max_var == stb->max_var);
        assert(sta->window_size == stb->window_size);
        double diversity = 0.0;
        for (int i = 1; i <= sta->max_var; i++) {
            diversity += std::abs(sta->get_decide_frequency(i) - stb->get_decide_frequency(i)) * std::abs(sta->get_phase_average(i) - stb->get_phase_average(i));
        }
        // 归一化处理：最大可能差异是max_var（每个变量相位差异最大为1.0）
        return diversity / sta->max_var;
    }


    // 构造函数：接收变量的最大编号和滑动窗口大小
    Statistics(int max_var, int window_size) 
        : max_var(max_var), window_size(window_size) {
        // 初始化数组，索引从1开始，所以大小为max_var+1
        decide_counts.resize(max_var + 1, 0);
        positive_counts.resize(max_var + 1, 0);
        initialized = false;
    }

    static void static_on_decide_callback(void* state, int decide_lit) {
        Statistics* self = static_cast<Statistics*>(state);
        self->on_decide(decide_lit);
        self->initialized = true;
    }
    
    // 回调函数：接收决策文字，更新统计信息
    void on_decide(int decide_lit) {
        int variable = var(decide_lit);
        
        // 更新决策计数
        decide_counts[variable]++;
        
        // 更新相位计数
        if (is_positive(decide_lit)) {
            positive_counts[variable]++;
        }
        
        // 将当前决策文字加入历史队列
        history_lits.push(decide_lit);
        
        // 如果队列超过窗口大小，移除最旧的决策并更新计数
        if (history_lits.size() > window_size) {
            int old_lit = history_lits.front();
            history_lits.pop();
            
            int old_var = var(old_lit);
            decide_counts[old_var]--;
            
            if (is_positive(old_lit)) {
                positive_counts[old_var]--;
            }
        }
    }

    bool isInitialized() const {
        return initialized;
    }
    
    // 获取变量在当前窗口内的决策频次
    int get_decide_frequency(int variable) const {
        if (variable <= 0 || variable > max_var) {
            return 0;
        }
        return decide_counts[variable];
    }
    
    // 获取变量在当前窗口内的正相位比例（0.0-1.0）
    double get_phase_average(int variable) const {
        if (variable <= 0 || variable > max_var || decide_counts[variable] == 0) {
            return 0.5; // 默认返回0.5表示中性
        }
        return static_cast<double>(positive_counts[variable]) / decide_counts[variable];
    }
    
    // 清空所有统计信息
    void clear() {
        std::fill(decide_counts.begin(), decide_counts.end(), 0);
        std::fill(positive_counts.begin(), positive_counts.end(), 0);
        
        // 清空历史队列
        while (!history_lits.empty()) {
            history_lits.pop();
        }
    }
    
    // 获取当前窗口大小
    int get_current_window_size() const {
        return history_lits.size();
    }
    
    // 获取最大窗口大小
    int get_max_window_size() const {
        return window_size;
    }
};