#include "yahs_sort.h"




// Graph类方法实现
int64_t Graph::add_arc(uint32_t v, uint32_t w, double wt, int64_t link_id, bool comp)
{
    link_id = link_id > 0 ? link_id : arcs.size();
    arcs.emplace_back(v, w, wt, link_id, comp);
    return this->arcs.size() - 1;
}


void Graph::arc_sort() 
{
    std::sort(arcs.begin(), arcs.end(),
              [](const GraphArc& a, const GraphArc& b) {
                  return a.v < b.v;
              });
}

/*
创建索引 idx 向量，
    编号为：节点编号<<32 | 正反向，
    每个元素为uint64_t类型，
        高32位：开始边的在arcs中的索引
        低32位：开始节点对应的边的数量
*/
void Graph::arc_index()
{
    this->idx.clear();
    this->idx.resize(arcs.size() * 2, 0);
    
    uint64_t last_i = 0;
    for (uint64_t i = 1; i <= arcs.size(); ++i) 
    {
        if (i == arcs.size() || arcs[i-1].v != arcs[i].v) 
        {
            this->idx[this->arcs[i-1].v] = (last_i << 32) | (i - last_i);
            last_i = i;
        }
    }
}

void Graph::clean(bool shear) {
    arcs.erase(
        std::remove_if(arcs.begin(), arcs.end(),
                      [](const GraphArc& arc) { return arc.del; }),
        arcs.end()
    );
    
    if (shear) {
        arcs.shrink_to_fit();
    }
    
    arc_sort();
    arc_index();
}


/*
    判断v节点是否存在权重大于wt的边，且该边指向w节点
*/
bool Graph::is_mwt(uint32_t v, uint32_t w) const {
    size_t na = get_arc_count(v);
    if (na == 0) return false;
    
    const GraphArc* av = get_arcs(v);
    double mwt = 0;
    bool found = false;
    
    for (size_t i = 0; i < na; ++i) {
        if (av[i].wt > mwt) {
            mwt = av[i].wt;
            found = (av[i].w == w);
        }
    }
    return found;
}


bool Graph::exist_strong_edge(uint32_t v, double wt) const {
    size_t na = get_arc_count(v);
    if (na < 2) return false;
    
    const GraphArc* av = get_arcs(v);
    for (size_t i = 0; i < na; ++i) {
        if (av[i].wt > wt) return true;
    }
    return false;
}

int Graph::trim_simple_filter(double min_wt, double min_diff_h, double min_diff_l, int min_len) {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    // 移除短序列的边
    for (uint32_t v = 0; v < nv; ++v) {
        if (v >= min_len) continue;
        size_t na = get_arc_count(v);
        GraphArc* av = get_arcs(v);
        for (size_t i = 0; i < na; ++i) {
            av[i].del = true;
            ++n_del;
        }
    }
    clean(true);
    
    // 基于权重过滤
    for (uint32_t v = 0; v < nv; ++v) {
        size_t na = get_arc_count(v);
        GraphArc* av = get_arcs(v);
        
        double mwt = 0, smwt = 0;
        size_t n_ma = 0;
        
        // 找出最大和次大权重
        for (size_t i = 0; i < na; ++i) {
            if (av[i].wt > mwt) {
                smwt = mwt;
                mwt = av[i].wt;
                n_ma = 1;
            } else if (av[i].wt == mwt) {
                ++n_ma;
            } else if (av[i].wt > smwt) {
                smwt = av[i].wt;
            }
        }
        
        // 过滤边
        for (size_t i = 0; i < na; ++i) {
            if ((av[i].wt >= min_wt && av[i].wt >= mwt * min_diff_h) ||
                (av[i].wt < min_wt && av[i].wt == mwt && mwt * min_diff_l >= smwt && n_ma == 1)) {
                continue;
            }
            av[i].del = true;
            ++n_del;
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_tips() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        if (get_arc_count(v^1) != 1 || get_arc_count(v) > 0) continue;
        
        GraphArc* av = get_arcs(v^1);
        if (get_arc_count(av->w^1) > 1 && !is_mwt(av->w^1, v)) {
            av->del = true;
            ++n_del;
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_blunts() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        if (get_arc_count(v^1) > 0) continue;
        
        size_t na = get_arc_count(v);
        if (na > 1) {
            GraphArc* av = get_arcs(v);
            bool del = true;
            
            for (size_t i = 0; i < na; ++i) {
                if (get_arc_count(av[i].w) < 1) {
                    del = false;
                    break;
                }
            }
            
            if (del) {
                for (size_t i = 0; i < na; ++i) {
                    av[i].del = true;
                    ++n_del;
                }
            }
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_repeats() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        if (get_arc_count(v) != 2 || get_arc_count(v^1) != 2) continue;
        
        GraphArc* av = get_arcs(v^1);
        uint32_t v1 = av->w;
        uint32_t v2 = (av + 1)->w;
        
        GraphArc* aw = get_arcs(v);
        uint32_t w1 = aw->w;
        uint32_t w2 = (aw + 1)->w;
        
        if ((is_mwt(v1, w1) && is_mwt(v2, w2)) ||
            (is_mwt(v1, w2) && is_mwt(v2, w1))) {
            av->del = true;
            (av + 1)->del = true;
            aw->del = true;
            (aw + 1)->del = true;
            n_del += 4;
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_transitive_edges() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        size_t na = get_arc_count(v);
        if (na < 2) continue;
        
        GraphArc* av = get_arcs(v);
        for (size_t i = 0; i < na; ++i) {
            for (size_t j = 0; j < na; ++j) {
                if (i == j) continue;
                if (is_mwt(av[i].w, av[j].w)) {
                    av[j].del = true;
                    ++n_del;
                }
            }
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_pop_bubbles() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        size_t na = get_arc_count(v);
        if (na != 2) continue;
        
        GraphArc* av = get_arcs(v);
        uint32_t v1 = av->w;
        uint32_t v2 = (av + 1)->w;
        
        if (get_arc_count(v1) == 1 && get_arc_count(v2) == 1) {
            GraphArc* av1 = get_arcs(v1);
            GraphArc* av2 = get_arcs(v2);
            
            if (av1->w == av2->w) {
                av->del = true;
                (av + 1)->del = true;
                av1->del = true;
                av2->del = true;
                n_del += 4;
            }
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_pop_undirected() {
    int n_del = 0;
    uint32_t nv = idx.size();
    
    for (uint32_t v = 0; v < nv; ++v) {
        if (get_arc_count(v) != 1) continue;
        
        GraphArc* av = get_arcs(v);
        uint32_t w = av->w;
        
        if (is_mwt(v^1, w) && get_arc_count(v^1) > 1 && get_arc_count(w) > 0) {
            ++n_del;
            // 删除互补边
            for (size_t i = 0; i < get_arc_count(w^1); ++i) {
                GraphArc* aw = get_arcs(w^1);
                if (aw[i].w == v) {
                    aw[i].del = true;
                    break;
                }
            }
            // 删除原边
            for (size_t i = 0; i < get_arc_count(v^1); ++i) {
                GraphArc* av1 = get_arcs(v^1);
                if (av1[i].w == w) {
                    av1[i].del = true;
                    break;
                }
            }
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_weak_edges() {
    int n_del = 0;
    
    for (auto& arc : arcs) {
        if (exist_strong_edge(arc.v, arc.wt) && exist_strong_edge(arc.w^1, arc.wt)) {
            arc.del = true;
            ++n_del;
        }
    }
    
    clean(true);
    return n_del;
}

int Graph::trim_self_loops() {
    int n_del = 0;
    
    for (auto& arc : arcs) {
        if (get_arc_count(arc.v) > 1) continue;
        
        for (size_t i = 0; i < get_arc_count(arc.w); ++i) {
            GraphArc* aw = get_arcs(arc.w);
            if (aw[i].w == arc.v) {
                aw[i].del = true;
                ++n_del;
                break;
            }
        }
    }
    
    clean(true);
    return n_del;
}


/* 
    删除歧义边
*/
int Graph::trim_ambiguous_edges() {
    int n_del = 0;
    
    for (auto& arc : arcs) {
        if (
            get_arc_count(arc.v) > 1      // 节点v存在多条边
            || get_arc_count(arc.w^1) > 1 // 节点w的互补边存在多条
        ) {
            arc.del = true;
            ++n_del;
        }
    }
    
    this->clean(true);
    return n_del;
}


// 构建图的方法
Graph* YaHS::build_graph_from_links(const LikelihoodTable& likelihood_table, double min_score, double la)
{
    auto g = std::make_unique<Graph>();
    
    uint32_t num_frags = likelihood_table.get_num_frags();
    
    // 预分配空间以提高性能
    g->reserve_arcs(num_frags * num_frags * 4);  // 最坏情况下每个片段对可能有4个方向的连接
    
    // 遍历所有可能的片段对构建图
    double link_score = 0.f;
    int index_arc = 0;
    for (uint32_t i = 0; i < num_frags; ++i) {
        for (uint32_t j = i+1; j < num_frags; ++j) {
            
            // 检查四个方向的连接
            for (uint32_t k = 0; k < 4; ++k) {
                link_score = likelihood_table(i, j, k);
                if (link_score >= min_score) {
                    // 添加边
                    index_arc = g->add_arc((i << 1) | (k >> 1), (j << 1) | (k & 1), link_score, -1, false);
                    
                    // 添加互补边
                    g->add_arc((j << 1) | !(k & 1), (i << 1) | !(k >> 1), link_score, index_arc, true);
                }
            }
        }
    }
    
    // 排序和索引
    g->arc_sort();
    g->arc_index();
    
    return g.release();
}



// 运行scaffolding
std::vector<std::vector<uint32_t>> YaHS::yahs_sorting(const LikelihoodTable& likelihood_table, double min_score, double la)
{
    min_norm_ = min_score;
    la_ = la;
    
    // 构建图
    std::unique_ptr<Graph> g(this->build_graph_from_links(likelihood_table, min_norm_, la_));
    
    // 记录修剪前的边数
    size_t n_arc = g->get_arc_size();
    
    // 循环进行图修剪，直到没有边被删除
    while (true) {
        // 简单过滤
        g->trim_simple_filter(0.1, 0.7, 0.1, 0);
        
        // 修剪端点
        g->trim_tips();
        
        // 修剪钝化
        g->trim_blunts();
        
        // 修剪重复边
        g->trim_repeats();
        
        // 修剪传递边
        g->trim_transitive_edges();
        
        // 修剪泡状边
        g->trim_pop_bubbles();
        
        // 修剪无向边
        g->trim_pop_undirected();
        
        // 修剪弱边
        g->trim_weak_edges();
        
        // 修剪自环边
        g->trim_self_loops();
        
        // 如果边数没有变化，说明修剪完成
        if (g->get_arc_size() == n_arc)
            break;
        n_arc = g->get_arc_size();
    }
    
    // 最后修剪歧义边
    g->trim_ambiguous_edges();
    
    // 构建染色体序列
    std::vector<std::vector<uint32_t>> chromosomes;
    std::vector<bool> visited(g->get_arc_size() * 2, false);
    
    // 遍历所有节点
    for (uint32_t v = 0; v < g->get_arc_size() * 2; ++v) {
        if (visited[v]) continue;
        
        // 开始一个新的染色体
        std::vector<uint32_t> chromosome;
        
        // 使用深度优先搜索遍历图
        uint32_t current = v;
        while (!visited[current]) {
            visited[current] = true;
            chromosome.push_back(current >> 1);  // 存储节点编号（去掉方向位）
            
            // 获取当前节点的边
            size_t na = g->get_arc_count(current);
            if (na == 0) break;
            
            const GraphArc* av = g->get_arcs(current);
            if (av == nullptr) break;
            
            // 选择权重最大的边
            double max_wt = -1;
            uint32_t next = current;
            
            for (size_t i = 0; i < na; ++i) {
                if (av[i].wt > max_wt) {
                    max_wt = av[i].wt;
                    next = av[i].w;
                }
            }
            
            if (next == current) break;
            current = next;
        }
        
        if (!chromosome.empty()) {
            chromosomes.push_back(std::move(chromosome));
        }
    }
    
    return chromosomes;
}

