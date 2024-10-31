#ifndef AISORT_H
#define AISORT_H

#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <memory>
#include <vector>

void sort_ai();


float32_t* cal_compressed_hic();


class AiModel {
public:
    AiModel(std::string model_path);
    ~AiModel();
    torch::Tensor organize_compressed_hic_into_graph(float32_t* compressed_hic, std::vector<int64_t> size);
    torch::Tensor predict_likelihood(torch::Tensor graph);
    std::vector<int> cal_curated_fragments_order(torch::Tensor likelihood_table);
private:
    torch::jit::Module model;
};

struct Ai_Model_Mask{
    std::string model_path;
    bool is_model_loaded=false;
    AiModel* model_ptr;

    Ai_Model_Mask()
        :model_path("/Users/sg35/PretextView/ai_model/model_scripted.pt"),
        is_model_loaded(false),
        model_ptr(nullptr) {}

    ~Ai_Model_Mask() {
        if (is_model_loaded) delete model_ptr;
    }

    void init_model() {
        if (is_model_loaded) return ;
        model_ptr = new AiModel(model_path);
        is_model_loaded = true;
    }

    bool model_valid() {
        if (!is_model_loaded) return false;
        try {
            torch::Tensor test_input = torch::rand({3, 10});
            auto test_output = model_ptr->predict_likelihood(test_input);
            return true;
        } catch (const c10::Error& e) {
            return false;
        }
    }
};





#endif // AISORT_H