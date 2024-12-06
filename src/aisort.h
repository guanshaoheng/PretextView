#ifndef AISORT_H
#define AISORT_H

#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <memory>
#include <vector>
#include "copy_texture.h"


struct Link {
    f32 score;
    u32 frag_a;
    u32 frag_b;
    u08 is_a_head_linked;
    u08 is_b_head_linked;
    Link(f32 score_, u32 frag_a_, u32 frag_b_, u08 is_a_head_linked_, u08 is_b_head_linked_)
        :score(score_), frag_a(frag_a_), frag_b(frag_b_), is_a_head_linked(is_a_head_linked_), is_b_head_linked(is_b_head_linked_) {}
};


struct LikelihoodTable
{
private:
    u32 num_frags;   // all the frags including the filtered out ones
    f32* data;       // with shape [num_frags, num_frags, 4]
public:
    Frag4compress* frags;
    LikelihoodTable(const GraphData* graph, const torch::Tensor& likelihood_tensor)
        :num_frags(graph->frags->num), frags(graph->frags), data(nullptr)
    {
        data = new f32[num_frags * num_frags * 4];
        for (u32 i = 0; i<num_frags * num_frags * 4; i++) data[i] = -1.f;
        this->set_data(likelihood_tensor, graph);
    }

    LikelihoodTable(Frag4compress* frags, const Matrix3D<f32>* compressed_hic, const f32 threshold=10.f/32769.f)
        :num_frags(frags->num), frags(frags), data(nullptr)
    {
        data = new f32[num_frags * num_frags * 4];
        for (u32 i = 0; i<num_frags * num_frags * 4; i++) data[i] = -1.f;
        for (u32 i = 0; i<num_frags; i++)
        {   
            if ((f32)frags->length[i]/(f32)frags->total_length <= threshold) continue;
            for (u32 j = 0; j < num_frags; j++)
            {   
                if ((f32)frags->length[j]/(f32)frags->total_length <= threshold) continue;
                (*this)(i, j, 0) = (*compressed_hic)(i, j, 0);
                (*this)(i, j, 1) = (*compressed_hic)(i, j, 1);
                (*this)(i, j, 2) = (*compressed_hic)(i, j, 3);
                (*this)(i, j, 3) = (*compressed_hic)(i, j, 2);
            }
        }
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
    u32 num_frags;                  // number of all the fragments
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
        u32 tmp = 0;
        for (auto chromosome : chromosomes) 
        {
            if (chromosome.size()!=0) tmp += 1;
        }
        num_chromosomes = tmp;
        num_frags_in_chromosomes = new u32[num_chromosomes];
        order = new s32*[num_chromosomes];
        u32 cnt= 0;
        for (u32 i = 0; i < chromosomes.size(); i++)
        {   
            if (chromosomes[i].size() == 0) continue;
            num_frags_in_chromosomes[cnt] = chromosomes[i].size();
            order[cnt] = new s32[num_frags_in_chromosomes[cnt]];
            for (u32 j = 0; j < num_frags_in_chromosomes[cnt]; j++)
            {
                order[cnt][j] = chromosomes[i][j];
                if (order[cnt][j] == 0)
                {
                    std::cerr << "Error: the order should not contain ("<< order[cnt][j] << ")." << std::endl;
                    assert(0);
                }
            }
            cnt++ ;
        }
        if (cnt != num_chromosomes)
        {
            fprintf(stderr, "The cnt(%d) != num_chromosomes(%d)\n", cnt, num_chromosomes);
            assert(0);
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

        #ifdef DEBUG // echo the results
            std::cout << "\n\nSorting results \n";
            for (u32 i = 0; i < num_chromosomes; i++)
            {   
                std::cout << "Chr [" << i+1 << "]: ";
                for (u32 j = 0; j < num_frags_in_chromosomes[i]; j++)
                {   
                    std::cout << order[i][j] << " ";
                }
                std::cout << std::endl;
            }
        #endif //DEBUG

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

    void sort_according_likelihood_dfs(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001);

    void sort_according_likelihood_unionFind(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr) const;

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