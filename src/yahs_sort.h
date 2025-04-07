#ifndef YAHS_SORT_H
#define YAHS_SORT_H

// likelihood table
#include "likelihood_table.h"
// frags_order
#include "frags_order.h"



// 定义图相关的结构体
struct GraphArc 
{
    uint32_t v;      // 前31位表示节点编号，最后一位表示方向
    uint32_t w;      // 前31位表示节点编号，最后一位表示方向
    double wt;       // 权重
    int64_t link_id; // 边的id 
    bool del=false;        // 是否删除
    bool comp;       // 是否互补

    GraphArc(uint32_t v, uint32_t w, double wt, int64_t link_id, bool comp)
        : v(v), w(w), wt(wt), link_id(link_id), comp(comp) {}
};

class Graph 
{
public:
    Graph() = default;
    ~Graph() = default;
    
    // 基本图操作方法
    // 添加边
    int64_t add_arc(uint32_t v, uint32_t w, double wt, int64_t link_id, bool comp = false) ;
    // 预分配空间
    void reserve_arcs(size_t size) {
        arcs.reserve(size);
    }
    // 获取边的数量
    size_t get_arc_count(uint32_t v) const {
        if (v >= idx.size()) return 0;
        return idx[v] & 0xFFFFFFFF; // 取低32位
    }
    // 获取边的指针
    GraphArc* get_arcs(uint32_t v) {
        if (v >= idx.size()) return nullptr;
        uint64_t offset = idx[v] >> 32;
        return &arcs[offset];
    }
    // 获取边的指针（常量版本）
    const GraphArc* get_arcs(uint32_t v) const {
        if (v >= idx.size()) return nullptr;
        uint64_t offset = idx[v] >> 32;
        return &arcs[offset];
    }
    // 获取边的数量 
    size_t get_arc_size() const { return arcs.size(); }

    // 图操作相关方法
    // 对边进行排序
    void arc_sort();
    // 对边进行索引
    void arc_index();
    // 清理图，删除被标记为删除的边
    void clean(bool shear);
    
    // 图修剪方法
    // 基于权重过滤
    int trim_simple_filter(double min_wt, double min_diff_h, double min_diff_l, int min_len);
    // 修剪端点
    int trim_tips();
    // 修剪钝化
    int trim_blunts();
    // 修剪重复边
    int trim_repeats();
    // 修剪传递边
    int trim_transitive_edges();
    // 修剪泡状边
    int trim_pop_bubbles();
    // 修剪无向边
    int trim_pop_undirected();
    // 修剪弱边
    int trim_weak_edges();
    // 修剪自环边
    int trim_self_loops();
    // 修剪歧义边
    int trim_ambiguous_edges();

private:
    std::vector<GraphArc> arcs;
    std::vector<uint64_t> idx;  // 用于快速查找节点的边, 记录每个节点开始和结束的边
    
    // 辅助方法
    bool is_mwt(uint32_t v, uint32_t w) const; // 判断v和w之间是否存在最大权重边
    bool exist_strong_edge(uint32_t v, double wt) const; // 判断v节点是否存在权重大于wt的边
};




class YaHS
{
    
private:
    double min_norm_;
    double la_;
    
public:
    YaHS() = default;
    
    // 构建图的方法
    Graph* build_graph_from_links(const LikelihoodTable& likelihood_table, double min_norm, double la);
    
    // 运行scaffolding
    std::vector<std::vector<uint32_t>> yahs_sorting(const LikelihoodTable& likelihood_table, double min_norm = 0.1, double la = 0.1);
};


#endif // YAHS_H 