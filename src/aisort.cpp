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
    this->sort_according_likelihood(likelihood_table, frags_order);

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


void AiModel::sort_according_likelihood(
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