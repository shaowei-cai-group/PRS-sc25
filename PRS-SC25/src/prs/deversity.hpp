// #pragma once

// #include <set>

// #include "options.hpp"
// #include "statistics.hpp"
// #include "solvers/kissat.hpp"

// class Deversity {
// public:
//     Deversity(std::vector<KissatSolver*> &solvers): solvers(solvers) {
//         scores.resize(OPT(threads));
//         for(int i=0; i<OPT(threads); i++) {
//             scores[i].resize(OPT(threads));
//         }
//     }

//     std::vector<std::vector<int>> generate_groups() {
//         recall_scores();
//         std::vector<std::vector<int>> groups;

//         std::set<int> thread_set;
//         for(int i=0; i<OPT(threads); i++) {
//             thread_set.insert(i);
//         }

//         while(!thread_set.empty()) {
//             // 创建新的组
//             if(groups.size() == 0 || groups.back().size() >= OPT(share_grps)) {
//                 groups.emplace_back();
//                 groups.back().push_back(*thread_set.begin());
//                 thread_set.erase(thread_set.begin());
//                 continue;
//             }

//             double max_score = -1;
//             double max_thread = -1;

//             for(auto it = thread_set.begin(); it != thread_set.end(); it++) {
//                 double min_score = 1e9;
//                 int thread_1 = *it;
//                 for(auto thread_2 : groups.back()) {
//                     min_score = std::min(min_score, scores[thread_1][thread_2]);
//                 }
//                 if(min_score > max_score) {
//                     max_score = min_score;
//                     max_thread = *it;
//                 }
//             }

//             groups.back().push_back(max_thread);
//             thread_set.erase(max_thread);
//         }
//         return groups;
//     }

//     void print_groups(std::vector<std::vector<int>> &groups) {
//         for(auto group : groups) {
//             for(auto thread : group) {
//                 printf("%d ", thread);
//             }

//             double min_diversity = 1e9;

//             for(auto thread_1 : group) {
//                 for(auto thread_2 : group) {
//                     if(thread_1 != thread_2) {
//                         min_diversity = std::min(min_diversity, scores[thread_1][thread_2]);
//                     }
//                 }
//             }

//             printf("(%.2f)", min_diversity);
//             printf(" | ");
//         }
//         printf("\n");
//     }


// private:
//     void recall_scores() {
//         double min_diversity = 1e9;
//         int min_diversity_thread_1 = -1;
//         int min_diversity_thread_2 = -1;

//         // 计算两两线程的多样性，输出矩阵
//         for (int i = 0; i < OPT(threads); i++) {
//             for (int j = 0; j < OPT(threads); j++) {
//                 if (i != j && solvers[i]->getStatistics() != nullptr && solvers[j]->getStatistics() != nullptr && solvers[i]->getStatistics()->isInitialized() && solvers[j]->getStatistics()->isInitialized()) {
//                     double diversity = Statistics::getDiversityScore(solvers[i]->getStatistics(), solvers[j]->getStatistics());
//                     if (diversity < min_diversity) {
//                         min_diversity = diversity;
//                         min_diversity_thread_1 = i;
//                         min_diversity_thread_2 = j;
//                     }
//                     // printf("%.2f ", diversity);
//                     scores[i][j] = diversity;
//                 } else {
//                     // printf("0.00 ");
//                     scores[i][j] = 0;
//                 }
//             }
//             // printf("\n");
//         }
//         // printf("\n");
//     }

//     std::vector<std::vector<double>> scores;
//     std::vector<KissatSolver*> &solvers;
// };