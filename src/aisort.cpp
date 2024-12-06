#include <aisort.h>


/* 
sort the map according to use the pre-trained model


*/
void sort_ai(){
    printf("sort_ai func loaded\n");

}


AiModel::AiModel()
        :model_path("/Users/sg35/PretextView/ai_model/model_scripted.pt"),
        is_model_loaded_and_valid(false) 
{
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
    const f32 threshold)
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
    std::vector<std::deque<s32>> chromosomes(n);
    for (u32 i=0; i < n; i++) chromosomes[i].push_back(i+1); 
    std::vector<u32> chain_id(n);
    std::iota(chain_id.begin(), chain_id.end(), 0);

    // Process links in order of decreasing score
    for (auto &lnk : all_links)
    {
        u32 a = lnk.frag_a;
        u32 b = lnk.frag_b;
        u32 ca = chain_id[a], cb = chain_id[b];

        if (ca == cb) continue;

        // Decide how to connect them:
        auto &cha = chromosomes[ca];
        auto &chb = chromosomes[cb];
        
        // if the fragments are not the head or end of the chain, 
        // thus they can not be linked
        if (!is_head_or_end(cha, a) || !is_head_or_end(chb, b)) continue;

        // sometimes, the fragment is the front one also the back one
        if (cha.size() == 1 && chb.size() == 1 )
        {
            if (lnk.is_a_head_linked && lnk.is_b_head_linked)
            {
                cha.push_front(-chb.front());
            }
            else if (lnk.is_a_head_linked && !lnk.is_b_head_linked)
            {
                cha.push_front(chb.front());
            }
            else if (!lnk.is_a_head_linked && lnk.is_b_head_linked)
            {
                cha.push_back(chb.front());
            }
            else // !lnk.is_a_head_linked && !lnk.is_b_head_linked
            {
                cha.push_back(-chb.front());
            }
            chain_id[b] = chain_id[a];
            chb.clear();
            continue;
        }
        else if (cha.size() == 1 && chb.size() != 1) // append a to b
        {
            if (!is_head_or_end(chb, b)) continue;
            bool is_b_front_placed = std::abs(chb.front())-1 == b;
            bool is_b_inversed = (is_b_front_placed? chb.front():chb.back()) < 0;
            if (!is_attachable(is_b_front_placed, is_b_inversed, lnk.is_b_head_linked)) continue;
            if (is_b_front_placed)
            {
                if (lnk.is_a_head_linked) chb.push_front(-cha.front());
                else chb.push_front(cha.front());
            }
            else
            {
                if (lnk.is_a_head_linked) chb.push_back(cha.front());
                else chb.push_back(-cha.front());
            }
            chain_id[a] = chain_id[b];
            cha.clear();
            continue;
        }
        else if (cha.size() != 1 && chb.size() == 1) // append b to a
        {
            if (!is_head_or_end(cha, a)) continue;
            bool is_a_front_placed = std::abs(cha.front())-1 == a;
            bool is_a_inversed = (is_a_front_placed? cha.front():cha.back()) < 0;
            if (!is_attachable(is_a_front_placed, is_a_inversed, lnk.is_a_head_linked)) continue;
            if (is_a_front_placed)
            {
                if (lnk.is_b_head_linked) cha.push_front(-chb.front());
                else cha.push_front(chb.front());
            }
            else
            {
                if (lnk.is_b_head_linked) cha.push_back(chb.front());
                else cha.push_back(-chb.front());
            }
            chain_id[b] = chain_id[a];
            chb.clear();
            continue;
        }

        // size is not only 1
        bool is_a_front_placed = std::abs(cha.front())-1 == a, 
             is_b_front_placed = std::abs(chb.front())-1 == b;
        bool is_a_inversed = (is_a_front_placed? cha.front():cha.back()) < 0,
             is_b_inversed = (is_b_front_placed? chb.front():chb.back()) < 0;
        if (!is_attachable(is_a_front_placed, is_a_inversed, lnk.is_a_head_linked) ||
            !is_attachable(is_b_front_placed, is_b_inversed, lnk.is_b_head_linked)) continue;
        if (!is_a_front_placed) // a back
        {   
            if (!is_b_front_placed) // b back
            {
                std::reverse(chb.begin(), chb.end());
                for (auto &val: chb) val = -val; // flip orientation
            }
            // append chb to cha
            for (auto &val: chb) {
                u32 frag_id = std::abs(val)-1;
                chain_id[frag_id] = ca;
                cha.push_back(val);
            }
            chb.clear();
        } else  // a front
        {   
            if (is_b_front_placed) // b front
            {
                std::reverse(chb.begin(), chb.end());
                for (auto &val: chb) val = -val; // flip orientation
            }
            // append cha to chb
            for (auto &val: cha) {
                u32 frag_id = std::abs(val)-1;
                chain_id[frag_id] = cb;
                chb.push_back(val);
            }
            cha.clear();
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
    }

    // Now, chromosomes vector holds chains formed by linking from highest score down
    frags_order.set_order(chromosomes);
}