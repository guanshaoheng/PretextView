#ifndef AISORT_H
#define AISORT_H

#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <memory>
#include <vector>

#include "copy_texture.h"


void sort_ai();


struct LikelihoodTable
{
private:
    u32 num_frags;   // all the frags including the filtered out ones
    f32* data;       // with shape [num, num, 4]
public:
    Frag4compress* frags;
    LikelihoodTable(const GraphData* graph, const torch::Tensor& likelihood_tensor)
        :num_frags(graph->frags->num), frags(graph->frags)
    {
        data = new f32[num_frags * num_frags * 4];
        for (u32 i = 0; i<num_frags * num_frags * 4; i++) data[i] = -1.f;
        this->set_data(likelihood_tensor, graph);
    }
    ~LikelihoodTable()
    {
        if (data) 
        {
            delete[] data;
            data = nullptr;
        }
    }

    u32 get_num_frags() const {return num_frags;}

    f32& operator() (const u32 i, const u32 j, const u32 k) 
    {   
        if (i >= num_frags || j >= num_frags || k >= 4) 
        {
            std::cerr << "Error: index out of range" << std::endl;
            assert(0);
        }
        return data[i * num_frags * 4 + j * 4 + k];
    }

    const f32& operator() (const u32 i, const u32 j, const u32 k) const
    {
        if (i >= num_frags || j >= num_frags || k >= 4) 
        {
            std::cerr << "Error: index out of range" << std::endl;
            assert(0);
        }
        return data[i * num_frags * 4 + j * 4 + k];
    }

    void set_data(
        const  torch::Tensor& likelihood_tensor, 
        const GraphData* graph_data)
    {   
        u32 num_selected_nodes = graph_data->get_num_nodes();
        std::vector data_(likelihood_tensor.data_ptr<f32>(), likelihood_tensor.data_ptr<f32>() + likelihood_tensor.numel()); // [num_edges, 4]
        u32 num_edges_of_selected_nodes = num_selected_nodes * (num_selected_nodes - 1) >> 1;
        if ((data_.size() >> 2) != num_edges_of_selected_nodes)
        {   
            fprintf(stderr, "The size of the data is not correct, %d(data_) != %d(num_edges_should_be)\n", (u32)data_.size() >> 2, num_edges_of_selected_nodes);
            std::cerr << "Error: the size of the data is not correct" << std::endl;
            assert(0);
        }

        auto frags_idx_new_to_old = graph_data->get_selected_node_idx();
        for (u32 i = 0; i < graph_data->get_num_full_edges(); i ++  )
        {   
            // TODO (SHAOHENG): copy the data to the likelihood table
            u32 node_i = graph_data->edges_index_full(0, i), node_j = graph_data->edges_index_full(1, i);
            if (node_i > node_j) 
            {
                fprintf(stderr, "The node_i(%d) should be less than node_j(%d)\n", node_i, node_j);
                assert(0);
            }
            if (node_j >= num_selected_nodes)
            {
                fprintf(stderr, "The node_j(%d) should be less than num_selected_nodes(%d)\n", node_j, num_selected_nodes);
                assert(0);
            }
            for (u32 k = 0; k < 4; k++)
            {
                (*this)(frags_idx_new_to_old[node_i], frags_idx_new_to_old[node_j], k) = data_[i * 4 + k];
            }
            (*this)(frags_idx_new_to_old[node_j], frags_idx_new_to_old[node_i], 0) = (*this)(frags_idx_new_to_old[node_i], frags_idx_new_to_old[node_j], 0);
            (*this)(frags_idx_new_to_old[node_j], frags_idx_new_to_old[node_i], 1) = (*this)(frags_idx_new_to_old[node_i], frags_idx_new_to_old[node_j], 2);
            (*this)(frags_idx_new_to_old[node_j], frags_idx_new_to_old[node_i], 2) = (*this)(frags_idx_new_to_old[node_i], frags_idx_new_to_old[node_j], 1);
            (*this)(frags_idx_new_to_old[node_j], frags_idx_new_to_old[node_i], 3) = (*this)(frags_idx_new_to_old[node_i], frags_idx_new_to_old[node_j], 3);

        }
    }

};


struct FragsOrder
{
private:
    u32 num_frags;
    u32 num_chromosomes;
    u32* num_frags_in_chromosomes;  // [num_chromosomes]
    s32** order;                    // [num_chromosomes, num_frags_in_chromosomes], + - represents the direction of the fragment, start from 1
    void cleanup()
    {   
        if (order) 
        {   
            for (u32 i = 0; i < num_chromosomes; i++)
            {
                if (order[i])
                {
                    delete[] order[i];
                    order[i] = nullptr;
                }
            }
            delete[] order;
            order = nullptr;
        }

        if (num_frags_in_chromosomes)
        {
            delete[] num_frags_in_chromosomes;
            num_frags_in_chromosomes = nullptr;
        }
        num_chromosomes = 0;
    }

public:
    FragsOrder(u32 num_frags_)
        :num_frags(num_frags_), num_chromosomes(0), num_frags_in_chromosomes(nullptr), order(nullptr)
    {}

    ~FragsOrder()
    {   
        cleanup();
    }
    
    u32 get_num_frags() const {return num_frags;}

    void set_order(const std::vector<std::deque<s32>> chromosomes)
    {   
        cleanup(); // make sure the memory is released
        num_chromosomes = chromosomes.size();
        num_frags_in_chromosomes = new u32[num_chromosomes];
        order = new s32*[num_chromosomes];
        for (u32 i = 0; i < num_chromosomes; i++)
        {
            num_frags_in_chromosomes[i] = chromosomes[i].size();
            order[i] = new s32[num_frags_in_chromosomes[i]];
            for (u32 j = 0; j < num_frags_in_chromosomes[i]; j++)
            {
                order[i][j] = chromosomes[i][j];
                if (order[i][j] == 0)
                {
                    std::cerr << "Error: the order should not contain ("<< order[i][j] << ")." << std::endl;
                    assert(0);
                }
            }
        }
    }

    std::vector<s32> get_order_without_chromosomeInfor() const
    {
        if (num_chromosomes == 0)
        {
            std::cerr << "Error: the order is not set yet" << std::endl;
            assert(0);
        }
        std::vector<s32> output;
        for (u32 i = 0; i < num_chromosomes; i++)
        {
            for (u32 j = 0; j < num_frags_in_chromosomes[i]; j++)
            {
                output.push_back(order[i][j]);
            }
        }

        std::cout << "\n\nAi sorting results \n";
        for (u32 i = 0; i < num_chromosomes; i++)
        {   
            std::cout << "Chr [" << i+1 << "]: ";
            for (u32 j = 0; j < num_frags_in_chromosomes[i]; j++)
            {   
                std::cout << order[i][j] << " ";
            }
            std::cout << std::endl;
        }

        return output;
    }
};


class AiModel 
{
private:
    std::string model_path;
    torch::jit::Module model;
public:
    bool is_model_loaded_and_valid;
    AiModel();

    ~AiModel();

    torch::Tensor predict_likelihood(
        const torch::Tensor& x, 
        const torch::Tensor& edge_index, 
        const torch::Tensor& edge_attr, 
        const torch::Tensor& edge_index_test);

    void cal_curated_fragments_order(
        const GraphData* graph, 
        FragsOrder& frags_order);

    void sort_according_likelihood(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001);

    void model_valid() 
    {
        try 
        {   
            torch::Tensor x = torch::rand({64, 8}), edge_index = torch::randint(64, {2, 5}, torch::kInt64), edge_attr=torch::rand({5, 7}), edge_index_test = torch::randint(64, {2, 5}, torch::kInt64);
            auto test_output = this->predict_likelihood(x, edge_index, edge_attr, edge_index_test);
            is_model_loaded_and_valid = true;
            std::cout << "[Ai model]: validated." << std::endl;
        } catch (const c10::Error& e) 
        {
            // Handle errors from TorchScript
            std::cerr << "Error: model is not loaded correctly. " << e.what() << std::endl;
            is_model_loaded_and_valid = false;
        } catch (const std::exception& e) 
        {
            // Handle other standard exceptions
            std::cerr << "Standard error occurred: " << e.what() << std::endl;
            is_model_loaded_and_valid = false;
        }
    }
};



#endif // AISORT_H