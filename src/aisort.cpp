
/*
Copyright (c) 2024 Shaoheng Guan, Wellcome Sanger Institute

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include "aisort.h"


/* 
sort the map according to use the pre-trained model


*/
void sort_ai(){
    printf("sort_ai func loaded\n");

}


AiModel::AiModel()
        :model_path("./ai_model/model_scripted.pt"),
        is_model_loaded_and_valid(false) 
{   
    printf("[AI Model] Loading the model from %s\n", model_path.c_str());
    try {
        model = torch::jit::load(model_path);
        model.eval();
        std::cout << "Model loaded successfully, from " << model_path << std::endl;
    } catch (const c10::Error& e) {
        std::cerr << "Error: loading the model from " << model_path << std::endl;
        throw std::runtime_error("Model loading error!");
    }
    model_valid();
}

AiModel::~AiModel() {
    std::cout << "AI model destructed." << std::endl;
}

torch::Tensor AiModel::predict_likelihood(
    const torch::Tensor& x, 
    const torch::Tensor& edge_index, 
    const torch::Tensor& edge_attr, 
    const torch::Tensor& edge_index_test)
{
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(x);
    inputs.push_back(edge_index);
    inputs.push_back(edge_attr);
    inputs.push_back(edge_index_test);
    torch::Tensor output_tensor = model.forward(inputs).toTensor();
    return output_tensor;
}

void AiModel::cal_curated_fragments_order(
    const GraphData* graph, 
    FragsOrder& frags_order) 
{   
    // Convert `nodes` to a Tensor (e.g., x)
    torch::Tensor x = torch::from_blob(
        (void*)graph->get_nodes_data_ptr(), // pointer to the data
        {graph->get_num_nodes(), graph->get_num_node_properties()}, // shape
        torch::kFloat32).clone();

    // Convert `edges_index` to a Tensor (e.g., edge_index)
    torch::Tensor edge_index = torch::from_blob(
        (void*)graph->get_edge_index_data_ptr(),
        {2, graph->get_num_edges()},
        torch::kUInt32).to(torch::kInt64).clone();

    // Convert `edges` to a Tensor (e.g., edge_attr)
    torch::Tensor edge_attr = torch::from_blob(
        (void*)graph->get_edges_data_ptr(),
        {graph->get_num_edges(), graph->get_num_edge_properties()},
        torch::kFloat32).clone();

    // Convert `edges_index_full` to a Tensor (e.g., edge_index_test)
    torch::Tensor edge_index_test = torch::from_blob(
        (void*)graph->get_edges_index_full_data_ptr(),
        {2, graph->get_num_full_edges()},
        torch::kUInt32).to(torch::kInt64).clone();

    // predict the likelihood
    torch::Tensor likelihood_tensor = this->predict_likelihood(x, edge_index, edge_attr, edge_index_test);
    LikelihoodTable likelihood_table(graph, likelihood_tensor); // TODO (shaoheng) copy likelihood_tensor to the likelihood_table

    // search the order 
    this->sort_according_likelihood_dfs(likelihood_table, frags_order);

    return ;
}


void dfs(
    u32 i, 
    std::vector<bool>& visited, 
    std::deque<s32>& order, 
    const f32& threshold, 
    const LikelihoodTable& likelihood_table,
    bool head_flag,
    bool inversed_flag)
{   
    s32 tmp_i;
    if (inversed_flag) tmp_i = - (s32)(i+1);
    else tmp_i = (s32)(i+1);
    if (head_flag) order.push_front(tmp_i);
    else order.push_back(tmp_i);
    visited[i] = true;

    // Check if all nodes are visited
    if (std::all_of(visited.begin(), visited.end(), [](bool v) { return v; }))  return;

    // find the best neighbor of the head and the end
    s32 max_id;
    f32 max_val = -1e9;
    head_flag = true;
    // from head
    u32 head_id = std::abs(order.front()) - 1;
    bool head_is_inversed = order.front() < 0;
    for (u32 j = 0; j < likelihood_table.get_num_frags(); j++)
    {   
        if ( j == head_id || visited[j] ) continue;

        if (!head_is_inversed) // check the start
        {
            if (likelihood_table(head_id, j, 0) > max_val) // link to the start
            {
                max_val = likelihood_table(head_id, j, 0);
                max_id = -(s32)j;
            }
            if (likelihood_table(head_id, j, 1) > max_val) // link to the end
            {
                max_val = likelihood_table(head_id, j, 1);
                max_id = (s32)j;
            }
        }
        else // check the end
        {
            if (likelihood_table(head_id, j, 2) > max_val) // link to the start
            {
                max_val = likelihood_table(head_id, j, 2);
                max_id = -(s32)j;
            }
            if (likelihood_table(head_id, j, 3) > max_val) // link to the end
            {
                max_val = likelihood_table(head_id, j, 3);
                max_id = (s32)j;
            }
        }
    }
    // from end
    u32 end_id = std::abs(order.back()) - 1;
    bool end_is_inversed = order.back() < 0;
    for (u32 j = 0; j < likelihood_table.get_num_frags(); j++)
    {
        if ( j == end_id || visited[j] ) continue;

        if (!end_is_inversed) // check the end
        {
            if (likelihood_table(end_id, j, 2) > max_val) // link to the start
            {
                max_val = likelihood_table(end_id, j, 2);
                max_id = (s32)j;
                head_flag = false;
            }
            if (likelihood_table(end_id, j, 3) > max_val) // link to the end
            {
                max_val = likelihood_table(end_id, j, 3);
                max_id = -(s32)j;
                head_flag = false;
            }
        }
        else // check the start
        {
            if (likelihood_table(end_id, j, 0) > max_val) // link to the start
            {
                max_val = likelihood_table(end_id, j, 0);
                max_id = (s32)j;
                head_flag = false;
            }
            if (likelihood_table(end_id, j, 1) > max_val) // link to the end
            {
                max_val = likelihood_table(end_id, j, 1);
                max_id = -(s32)j;
                head_flag = false;
            }
        }
    }

    // check the threshold
    if (max_val < threshold) return;
    
    // dfs
    inversed_flag = max_id < 0;
    dfs(std::abs(max_id), visited, order, threshold, likelihood_table, head_flag, inversed_flag);
    
    return ;
}


void AiModel::sort_according_likelihood_dfs(
    const LikelihoodTable& likelihood_table, 
    FragsOrder& frags_order, 
    const f32 threshold, 
    const Frag4compress* frags) const 
{

    // sort the fragments according to length

    // search for the order via dfs
    std::vector<bool> visited(likelihood_table.get_num_frags(), false);
    std::vector<std::deque<s32>> chromosomes;
    for (u32 i = 0; i < likelihood_table.get_num_frags(); i++)
    {
        if (visited[i]) continue;
        if (std::all_of(visited.begin(), visited.end(), [](bool v) { return v; }))  break;

        std::deque<s32> chromosome;
        dfs(i, visited, chromosome, threshold, likelihood_table, true, false);
        chromosomes.push_back(chromosome);
    }

    // sort the chromosomes according to the length
    if (frags)
    {
        std::sort(
            chromosomes.begin(), 
            chromosomes.end(), 
            [&frags](const std::deque<s32>& a, const std::deque<s32>& b){
                u32 len_a = 0, len_b = 0;
                for (auto i : a) len_a += frags->length[std::abs(i)-1];
                for (auto i : b) len_b += frags->length[std::abs(i)-1];
                return len_a > len_b;
            });
    }

    // set the order
    frags_order.set_order(chromosomes);

    return ;
}


bool is_head_or_end(const std::deque<s32>& chromosome, const u32& a)
{
    if (std::abs(chromosome.front())-1 == a || std::abs(chromosome.back())-1 == a) return true;
    else return false;
}


// decide whether fragment a can be attached with the link
bool is_attachable(bool is_front, bool is_inversed, bool is_head_linked)
{
    if ((is_front && is_inversed) || (!is_front && !is_inversed)) return is_head_linked? false:true; // end available
    else if ((is_front && !is_inversed) || (!is_front && is_inversed)) return is_head_linked? true:false; // start available
    else
    {
        std::cerr << "Error: is_attachable" << std::endl;
        assert(0);
    }
    return false;
}


void initilise_chromosomes(
    std::vector<std::deque<s32>>& chromosomes, 
    std::vector<std::deque<s32>>& chromosomes_excluded, 
    std::vector<s32>& chain_id, 
    const u32& n, // total number of fragments
    const LikelihoodTable& likelihood_table)
{
    for (u32 i=0; i < n; i++) 
    {
        if (likelihood_table.excluded_fragment_idx.count(i)!=0)  // excluded fragment
        {
            chromosomes_excluded.push_back({(s32)i+1});
        }
        else
        {
            chain_id[i] = chromosomes.size();
            chromosomes.push_back({(s32)i+1});
        }
    }
}


void AiModel::sort_according_likelihood_unionFind(
    const LikelihoodTable& likelihood_table, 
    FragsOrder& frags_order, 
    const f32 threshold, 
    const Frag4compress* frags) const 
{
    u32 n = likelihood_table.get_num_frags();
    std::vector<Link> all_links;

    std::vector<std::pair<bool, bool>> link_kinds = {
        {true,  true}, {true,  false}, 
        {false, true}, {false, false}};
    for (u32 i = 0; i < n; i++) 
    {
        for (u32 j = i+1; j < n; j++)
        {
            for (u32 k = 0; k < 4; k++)
            {   
                f32 val = likelihood_table(i, j, k);
                if (val >= threshold) {
                    all_links.push_back(
                        Link(val, i, j, link_kinds[k].first, link_kinds[k].second));
                }
            }
        }
    }

    // Sort all links by descending score
    std::sort(all_links.begin(), all_links.end(), [](const Link& a, const Link& b) {
        return a.score > b.score;
    });
    // initialize the chromosomes and union-find
    // every fragment is initilised as a not-inverted chain
    std::vector<std::deque<s32>> chromosomes;
    std::vector<s32> chain_id(n, -1);
    std::vector<std::deque<s32>> chromosomes_excluded;
    initilise_chromosomes(chromosomes, chromosomes_excluded, chain_id, n, likelihood_table);

    // Process links in order of decreasing score
    for (auto &lnk : all_links)
    {
        u32 a = lnk.frag_a;
        u32 b = lnk.frag_b;
        s32 ca = chain_id[a], cb = chain_id[b];

        if (ca < 0 || cb < 0) 
        {
            fprintf(stderr, "Error: ca(%d) or cb(%d) should >= 0, only chain_id of excluded fragment can be smaller than 0.\n", ca, cb);
            assert(0);
        }

        if (ca == cb) continue;

        // Decide how to connect them:
        auto &cha = chromosomes[ca];
        auto &chb = chromosomes[cb];
        
        // if the fragments are not the head or end of the chain, 
        // thus they can not be linked
        if (!is_head_or_end(cha, a) || !is_head_or_end(chb, b)) continue;

        // find the index i/j of a/b on cha/b
        u32 i = 0, j = 0;
        for (; i < cha.size(); i++)
            if (std::abs(cha[i]) == a+1)
                break;
        for (; j < chb.size(); j++)
            if (std::abs(chb[j]) == b+1)
                break;
        if (i == cha.size() || j == chb.size())
        {
            fprintf(stderr, "Error: i(%d should no more than %zu)/j(%d should no more than %zu) not found\n", i, cha.size(), j, chb.size());
            assert(0);
        }
        
        // Reverse a and/or b if necessary to make it always a->b
        if (lnk.is_a_head_linked != (cha[i] < 0))
        {
            std::reverse(cha.begin(), cha.end());
            for (auto &val: cha) val = -val; // flip orientation
            i = cha.size() - 1 - i;
        }
        if (lnk.is_b_head_linked != (chb[j] > 0))
        {
            std::reverse(chb.begin(), chb.end());
            for (auto &val: chb) val = -val; // flip orientation
            j = chb.size() - 1 - j;
        }
        
        // join cha and chb if a is the last and b is the first
        if (i == cha.size() - 1 && j == 0) 
        {
            std::copy(chb.begin(), chb.end(), std::back_inserter(cha));
            for (auto &val: chb)
                chain_id[std::abs(val)-1] = ca;
            chb.clear();
        }
    }

    // sort the chromosomes according to the length
    if (frags)
    {
        std::sort(chromosomes.begin(), chromosomes.end(), 
            [&frags](const std::deque<s32>& a, const std::deque<s32>& b) {
                u32 len_a = 0, len_b = 0;
                for (auto &val: a) len_a += frags->length[std::abs(val)-1];
                for (auto &val: b) len_b += frags->length[std::abs(val)-1];
                return len_a > len_b;
            });
        
        std::sort(chromosomes_excluded.begin(), chromosomes_excluded.end(), 
            [&frags](const std::deque<s32>& a, const std::deque<s32>& b) {
                u32 len_a = 0, len_b = 0;
                for (auto &val: a) len_a += frags->length[std::abs(val)-1];
                for (auto &val: b) len_b += frags->length[std::abs(val)-1];
                return len_a > len_b;
            });
    }
    // merge two vectors
    chromosomes.insert(chromosomes.end(), chromosomes_excluded.begin(), chromosomes_excluded.end());

    frags_order.set_order(chromosomes);
}


// Chenxi Zhou
f32 link_score(
    const std::vector<Link>& all_links,
    const std::map<u64,u32>& map_links,
    s32 source, s32 sink)
{
    if (source == 0 || sink == 0)
        return 0;
    u64 k = (u64)(std::abs(source)<<1|(source<0?1:0))<<32|(std::abs(sink)<<1|(sink<0?1:0));
    auto it = map_links.find(k);
    if (it != map_links.end())
        return all_links[it->second].score;
    return -std::numeric_limits<f32>::infinity();
}


// Chenxi Zhou
std::deque<s32> fuse_chromosomes(
    const std::vector<Link>& all_links,
    const std::map<u64,u32>& map_links,
    std::deque<s32>& cha,
    std::deque<s32>& chb,
    s32 source, s32 sink,
    const f32 threshold)
    /*
    cha:    tail of a
    chb:    head of b
    source: cha[i]
    sink:   chb.size() > (j+1) ? chb[j+1] : 0
    */
{
    int n = cha.size();
    int m = chb.size();
    
    // push source
    cha.push_front(source);
    chb.push_front(source);

    // need two DP matrices
    // one for DP[i,j] ending with i-th element in cha
    // one for DP[i,j] ending with j-th element in chb
    // fill matrices row by row
    std::vector<double> pre_row_a(m+1, -std::numeric_limits<double>::infinity());   // i - 1
    std::vector<double> curr_row_a(m+1, -std::numeric_limits<double>::infinity());  // i
    std::vector<double> pre_row_b(m+1, -std::numeric_limits<double>::infinity());   // i - 1
    std::vector<double> curr_row_b(m+1, -std::numeric_limits<double>::infinity());  // i

    // backtracking matrix could be big - n*m*2
    // use one bit per position
    // tracking if need to switch list
    std::vector<std::vector<uint8_t>> bt(2, std::vector<uint8_t>(((int64_t)n*m-1)/8+1, 0));

    pre_row_a[0] = 0;
    pre_row_b[0] = 0;
    for (int j = 1; j <= m; ++j)  // initialise the score of the original chb
        pre_row_b[j] = pre_row_b[j-1] + link_score(all_links, map_links, chb[j-1], chb[j]);
    
    // fill DP and backtracking matrices
    double link1, link2;
    int64_t b;
    for (int i = 1; i <= n; ++i)  // check all the elements in cha
    {
        curr_row_a[0] = pre_row_a[0] + link_score(all_links, map_links, cha[i-1], cha[i]); // i = i, j = 0
        curr_row_b[0] = -std::numeric_limits<double>::infinity();                          // i = i, j = 0  // original code
        // curr_row_b[0] = pre_row_a[0] ; // i = i, j = 0  // changed code
        for (int j = 1; j <= m; ++j) {
            b = (i-1)*m + (j-1);
            link1 = pre_row_a[j] + link_score(all_links, map_links, cha[i-1], cha[i]);    // last is cha[i-1]
            link2 = pre_row_b[j] + link_score(all_links, map_links, chb[j], cha[i]);      // last is chb[j]
            curr_row_a[j] = std::max(link1, link2);
            bt[0][b/8] |= (uint8_t)(link1<link2?1:0)<<(b%8);
            link1 = curr_row_a[j-1] + link_score(all_links, map_links, cha[i], chb[j]);   // last is cha[i]
            link2 = curr_row_b[j-1] + link_score(all_links, map_links, chb[j-1], chb[j]); // last is chb[j-1]
            curr_row_b[j] = std::max(link1, link2);
            bt[1][b/8] |= (uint8_t)(link1<link2?0:1)<<(b%8);
        }
        std::swap(curr_row_a, pre_row_a);
        std::swap(curr_row_b, pre_row_b);
    }

    double link, maxLink;
    link = 0;
    for (int i = 1; i < cha.size(); ++i)
        link += link_score(all_links, map_links, cha[i-1], cha[i]);
    for (int j = 2; j < chb.size(); ++j)
        link += link_score(all_links, map_links, chb[j-1], chb[j]);
    link += link_score(all_links, map_links, chb.back(), sink);
        
    link1 = pre_row_a[m] + link_score(all_links, map_links, cha[n], sink);
    link2 = pre_row_b[m] + link_score(all_links, map_links, chb[m], sink);
    maxLink = std::max(link1, link2);

    // link is not strong enough
    if (maxLink < link + threshold)
        return std::deque<s32>();

    // pop source
    cha.pop_front();
    chb.pop_front();

    // do backtracking
    std::deque<s32> fused(0);
    std::array<std::reference_wrapper<std::deque<s32>>, 2> lists = {cha, chb};
    std::array<int, 2> sizes = {n-1, m-1};
    uint8_t k = link1<link2? 1 : 0;
    while(sizes[0] >= 0 && sizes[1] >= 0) // push cha and chb to fused according to backtracking matrix
    {
        b = sizes[0]*m + sizes[1];
        fused.push_front(lists[k].get()[sizes[k]--]);
        k ^= !!(bt[k][b/8]&((uint8_t)1<<(b%8)));
    }
    while(sizes[0] >= 0)
        fused.push_front(lists[0].get()[sizes[0]--]);
    while(sizes[1] >= 0)
        fused.push_front(lists[1].get()[sizes[1]--]);
    
    return fused;
}


// from Chenxi's code
std::deque<s32> fuse_chromosomes_shguan(
    const std::vector<Link>& all_links,
    const std::map<u64, u32>& map_links,
    std::deque<s32>& cha,
    std::deque<s32>& chb,
    s32 source, 
    s32 sink,
    const f32 threshold
)
{
    u32 n = cha.size(), m = chb.size();
    cha.push_front(source);
    chb.push_front(source);

    std::vector<std::vector<f32>> dp_a(n+1, std::vector<f32>(m+1, -std::numeric_limits<f32>::infinity())); // highest score end with cha[i]
    std::vector<std::vector<f32>> dp_b(n+1, std::vector<f32>(m+1, -std::numeric_limits<f32>::infinity())); // highest score end with chb[j]

    dp_a[0][0] = dp_b[0][0] = 0;
    
    for (u32 j = 1; j <= m; j++)
    {
        dp_b[0][j] = dp_b[0][j-1] + link_score(all_links, map_links, chb[j-1], chb[j]);
    }

    std::vector<std::vector<u64>> bt(2, std::vector<u64>(((u64)n * m + 7) / 8 , 0)); // track switch 
    
    f32 link1, link2;
    u32 cnt = 0;
    for (u32 i = 1; i <= n; i++)
    {   
        dp_a[i][0] = dp_a[i-1][0] + link_score(all_links, map_links, cha[i-1], cha[i]);
        for (u32 j = 1; j <= m; j++)
        {   
            cnt = (i-1) * m + (j-1);

            // process dp_a[i][j]
            link1 = dp_a[i-1][j] + link_score(all_links, map_links, cha[i-1], cha[i]);
            link2 = dp_b[i-1][j] + link_score(all_links, map_links, chb[j],   cha[i]);
            dp_a[i][j] = std::max(link1, link2);
            bt[0][cnt/8] |= (u64)(link1 < link2 ? 1 : 0) << (cnt % 8); // true: switched from chb, false: from cha
            // process dp_b[i][j]
            link1 = dp_a[i][j-1] + link_score(all_links, map_links, cha[i],   chb[j]);
            link2 = dp_b[i][j-1] + link_score(all_links, map_links, chb[j-1], chb[j]);
            dp_b[i][j] = std::max(link1, link2);
            bt[1][cnt/8] |= (u64)(link1 < link2 ? 0 : 1) << (cnt % 8); // true: switched from cha, false: from chb
        }
    }

    f32 link, maxLink;
    link = 0;
    for (int i = 1; i < cha.size(); ++i)
        link += link_score(all_links, map_links, cha[i-1], cha[i]);
    for (int j = 2; j < chb.size(); ++j)
        link += link_score(all_links, map_links, chb[j-1], chb[j]);
    link += link_score(all_links, map_links, chb.back(), sink);
        
    link1 = dp_a[n][m] + link_score(all_links, map_links, cha[n], sink);
    link2 = dp_b[n][m] + link_score(all_links, map_links, chb[m], sink);
    maxLink = std::max(link1, link2);

    // link is not strong enough
    if (maxLink < link + threshold)
        return std::deque<s32>();

    // pop source
    cha.pop_front(); // n
    chb.pop_front(); // m

    // do backtracking
    std::deque<s32> fused(0);
    std::array<std::reference_wrapper<std::deque<s32>>, 2> lists = {cha, chb};
    std::array<s32, 2> sizes = {(s32)n - 1, (s32)m - 1};
    uint8_t k = link1>link2? 0 : 1;
    while(sizes[0] >= 0 && sizes[1] >= 0) // push cha and chb to fused according to backtracking matrix
    {
        cnt = sizes[0]*m + sizes[1];
        fused.push_front(lists[k].get()[sizes[k]--]);
        k ^= !!(bt[k][cnt/8]&((uint8_t)1<<(cnt%8))); // check if current step is swicthed from the other chain
    }
    while(sizes[0] >= 0)
        fused.push_front(lists[0].get()[sizes[0]--]);
    while(sizes[1] >= 0)
        fused.push_front(lists[1].get()[sizes[1]--]);
    
    return fused;
}


// Chenxi Zhou
void AiModel::sort_according_likelihood_unionFind_doFuse(
    const LikelihoodTable& likelihood_table, 
    FragsOrder& frags_order, 
    const f32 threshold, 
    const Frag4compress* frags,
    const bool doStageOne,
    const bool doStageTwo) const 
{   
    u32 n = likelihood_table.get_num_frags();
    std::vector<Link> all_links;
    std::map<u64, u32> map_links;
    
    std::vector<std::pair<bool, bool>> link_kinds = {
        {true,  true}, {true,  false}, 
        {false, true}, {false, false}};
    for (u32 i = 0; i < n; i++) 
    {
        for (u32 j = i+1; j < n; j++)
        {
            for (u32 k = 0; k < 4; k++)
            {   
                f32 val = likelihood_table(i, j, k);
                if (val >= threshold) {
                    all_links.push_back(
                        Link(val, i, j, link_kinds[k].first, link_kinds[k].second));
                }
            }
        }
    }

    // Sort all links by descending score
    std::sort(all_links.begin(), all_links.end(), [](const Link& a, const Link& b) {
        return a.score > b.score;
    });
    if (doStageTwo) 
    {
        // Map sort links to frag pairs  (frga<<1|oria)<<32|(frgb<<1|orib)
        for (u32 i = 0; i < all_links.size(); i++)
        {
            auto &lnk = all_links[i];
            map_links[(u64)((lnk.frag_a+1)<<1|(lnk.is_a_head_linked?1:0))<<32|((lnk.frag_b+1)<<1|(lnk.is_b_head_linked?0:1))] = i;
            map_links[(u64)((lnk.frag_b+1)<<1|(lnk.is_b_head_linked?1:0))<<32|((lnk.frag_a+1)<<1|(lnk.is_a_head_linked?0:1))] = i; 
        }
    }
    // initialize the chromosomes and union-find
    // every fragment is initilised as a not-inverted chain
    std::vector<std::deque<s32>> chromosomes;
    std::vector<s32> chain_id(n, -1);
    std::vector<std::deque<s32>> chromosomes_excluded;
    initilise_chromosomes(chromosomes, chromosomes_excluded, chain_id, n, likelihood_table);

    // Process links in order of decreasing score
    if (doStageOne)
    {
        // Stage I end to end join
        for (auto &lnk : all_links)
        {
            u32 a = lnk.frag_a;
            u32 b = lnk.frag_b;
            u32 ca = chain_id[a], cb = chain_id[b];

            if (ca == cb) continue;

            // Decide how to connect them:
            auto &cha = chromosomes[ca];
            auto &chb = chromosomes[cb];
            
            // Find the index i/j of a/b on cha/b
            u32 i = 0, j = 0;
            for (; i < cha.size(); i++)
                if (std::abs(cha[i]) == a+1)
                    break;
            for (; j < chb.size(); j++)
                if (std::abs(chb[j]) == b+1)
                    break;
            
            // Reverse a and/or b if necessary to make it always a->b
            if (lnk.is_a_head_linked != (cha[i] < 0))
            {
                std::reverse(cha.begin(), cha.end());
                for (auto &val: cha) val = -val; // flip orientation
                i = cha.size() - 1 - i;
            }
            if (lnk.is_b_head_linked != (chb[j] > 0))
            {
                std::reverse(chb.begin(), chb.end());
                for (auto &val: chb) val = -val; // flip orientation
                j = chb.size() - 1 - j;
            }
            
            // join cha and chb if a is the last and b is the first
            if (i == cha.size() - 1 && j == 0) 
            {
                std::copy(chb.begin(), chb.end(), std::back_inserter(cha));
                for (auto &val: chb)
                    chain_id[std::abs(val)-1] = ca;
                chb.clear();
            }
        }
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    if (doStageTwo)
    {
        // Stage II make extra join by fusing chromosomes
        for (auto &lnk : all_links)
        {
            u32 a = lnk.frag_a;
            u32 b = lnk.frag_b;
            u32 ca = chain_id[a], cb = chain_id[b];

            if (ca == cb) continue;

            // Decide how to connect them:
            auto &cha = chromosomes[ca];
            auto &chb = chromosomes[cb];
            
            // Find the index i/j of a/b on cha/b
            u32 i = 0, j = 0;
            for (; i < cha.size(); i++)
                if (std::abs(cha[i]) == a+1)
                    break;
            for (; j < chb.size(); j++)
                if (std::abs(chb[j]) == b+1)
                    break;
            
            // Reverse a and/or b if necessary to make it always a->b
            if (lnk.is_a_head_linked != (cha[i] < 0))
            {
                std::reverse(cha.begin(), cha.end());
                for (auto &val: cha) val = -val; // flip orientation
                i = cha.size() - 1 - i;
            }
            if (lnk.is_b_head_linked != (chb[j] > 0))
            {
                std::reverse(chb.begin(), chb.end());
                for (auto &val: chb) val = -val; // flip orientation
                j = chb.size() - 1 - j;
            }

            // Now try to fuse cha and chb at position i and j
            // the fragments (i...end] on cha
            // and the fragments [beg...j) on chb
            // the sequence order in each chromosome keep unchanged
            // are fused to maximum the overall score (measured by link strength)
            std::deque<s32> atail(0);
            if (cha.size() > i+1)
                std::copy(cha.begin()+i+1, cha.end(), std::back_inserter(atail));
            std::deque<s32> bhead(chb.begin(), chb.begin()+j+1);

            std::deque<s32> fused = fuse_chromosomes_shguan(
                all_links, 
                map_links, 
                atail,                       // cha 
                bhead,                       // chb
                cha[i],                      // source
                chb.size()>(j+1)?chb[j+1]:0, // sink 
                threshold);
            if (!fused.empty()) 
            {
                if (cha.size() > i+1) 
                    cha.erase(cha.begin()+i+1, cha.end());                          // remove tail of cha
                std::copy(fused.begin(), fused.end(), std::back_inserter(cha));     // append the fused to cha
                if (chb.size() > j+1)  
                    std::copy(chb.begin()+j+1, chb.end(), std::back_inserter(cha)); // append the rest of chb to cha
                for (auto &val: chb)                                                // update chain_id of fragments in chb 
                    chain_id[std::abs(val)-1] = ca;
                chb.clear();
            }
        }
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<f64> elapsed_seconds = end_time-start_time;
    fprintf(stdout, "[Fusing]: consumed time %.2fs\n", elapsed_seconds.count());

    // sort the chromosomes according to the length
    if (frags)
    {
        std::sort(chromosomes.begin(), chromosomes.end(), 
            [&frags](const std::deque<s32>& a, const std::deque<s32>& b) {
                u32 len_a = 0, len_b = 0;
                for (auto &val: a) len_a += frags->length[std::abs(val)-1];
                for (auto &val: b) len_b += frags->length[std::abs(val)-1];
                return len_a > len_b;
            });
        
        std::sort(chromosomes_excluded.begin(), chromosomes_excluded.end(), 
            [&frags](const std::deque<s32>& a, const std::deque<s32>& b) {
                u32 len_a = 0, len_b = 0;
                for (auto &val: a) len_a += frags->length[std::abs(val)-1];
                for (auto &val: b) len_b += frags->length[std::abs(val)-1];
                return len_a > len_b;
            });
    }
    // merge two vectors
    chromosomes.insert(chromosomes.end(), chromosomes_excluded.begin(), chromosomes_excluded.end());

    frags_order.set_order(chromosomes);
}