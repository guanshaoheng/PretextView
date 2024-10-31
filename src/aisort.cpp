#include <aisort.h>


/* 
sort the map according to use the pre-trained model


*/
void sort_ai(){
    printf("sort_ai func loaded\n");

}

float32_t* cal_compressed_hic() {
    float32_t* compressed_hic = new float32_t[10];
    for (int i = 0; i < 10; i++) {
        compressed_hic[i] = (float32_t)i;
    }
    return compressed_hic;
}


AiModel::AiModel( std::string model_path) {
    try {
        model = torch::jit::load(model_path);
        model.eval();
        std::cout << "Model loaded successfully, from " << model_path << std::endl;
    } catch (const c10::Error& e) {
        std::cerr << "Error: loading the model from " << model_path << std::endl;
        throw std::runtime_error("Model loading error!");
    }
}

AiModel::~AiModel() {
    std::cout << "model released" << std::endl;
}

torch::Tensor AiModel::organize_compressed_hic_into_graph(
    float32_t* compressed_hic, 
    std::vector<int64_t> size) {
    return torch::from_blob(compressed_hic, size, torch::kFloat);
}

torch::Tensor AiModel::predict_likelihood(torch::Tensor graph) {
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(graph);
    auto output = model.forward(inputs).toTensor();
    return output;
}

std::vector<int> AiModel::cal_curated_fragments_order(torch::Tensor likelihood_table) {
    std::vector<int> output = {1, 2, 3};
    return output;
}