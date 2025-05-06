#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "fmt/core.h"
#include "likelihood_table.h"

namespace py = pybind11;


template<typename T>
py::array_t<T> create_tensor(const int n, const int channel, const T* data_ptr) {
    // 总元素个数
    size_t total_size = n * n * channel;

    // 指定 shape 和 strides
    std::vector<ssize_t> shape = {n, n, channel};
    std::vector<ssize_t> strides = {
        static_cast<ssize_t>(n * channel * sizeof(T)),  // 跨 n 行
        static_cast<ssize_t>(channel * sizeof(T)),      // 跨 n 列
        static_cast<ssize_t>(sizeof(T))           // 跨特征维
    };

    return py::array_t<T>(shape, strides, data_ptr);
}



class KmeansClusters
{
    public:
    bool is_init = false;
    py::module kmeans_utils;
    KmeansClusters()
    {   
        try
        {
            // 将当前目录加入 Python 模块搜索路径
            py::module sys = py::module::import("sys");
            py::module os = py::module::import("os");
            #ifdef DEBUG
                sys.attr("path").attr("append")("./build_cmake");
            #else // NDEBUG
                #ifdef __APPLE__
                    sys.attr("path").attr("append")("./PretextViewAI.app/Contents/Resources");
                #elif defined(__linux__) 
                    sys.attr("path").attr("append")(".");
                #else // _WIN32 _WIN64
                    sys.attr("path").attr("append")(".");
                #endif 
            #endif // DEBUG
            std::string current_dir = os.attr("getcwd")().cast<std::string>();
            fmt::print("Current dir: {}\n",  current_dir);

            fmt::print("PythonPaths: ");
            auto python_paths = sys.attr("path");
            for (auto& path : python_paths)
            {
                fmt::print("\t{}\n", path.cast<std::string>());
            }

            // 导入 Python 模块
            kmeans_utils = py::module::import("kmeans_utils");
            fmt::print("Python kmeans module loaded successfully\n");
            is_init = true;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        
    }


    void print_clusters(const std::vector<std::vector<int>>& clusters) const
    {   
        for (int i = 0; i < clusters.size(); ++i)
        {   
            if (clusters[i].empty()) continue;
            fmt::print("cluster id: {}, size: {}, frags: \t", i, clusters[i].size());
            for (auto& frag_id : clusters[i])
            {
                fmt::print("{}, ", frag_id);
            }
            fmt::print("\n");
        }
    }


    std::vector<std::vector<int>> kmeans_func(const int k, const Matrix3D<f32>* mat3d=nullptr)
    {   
        if (!is_init)
        {
            std::cerr << "Error: KmeansClusters not initialized" << std::endl;
            assert(0);
        }
        py::array_t<int> labels  ;

        // cal py function to get cluster labels
        py::array_t<float> adj_matrix = create_tensor<float>(mat3d->row_num, mat3d->layer_num, mat3d->get_data_ptr());
        labels = kmeans_utils.attr("run_kmeans")(k, adj_matrix);

        // 解析 NumPy 数组结果
        py::buffer_info buf = labels.request();
        int* cluster_ids = static_cast<int*>(buf.ptr);
        std::vector<std::vector<int>> clusters(k);  
        for (int i = 0; i < buf.size; ++i)
        {   
            if (cluster_ids[i] < 0 || cluster_ids[i] >= k)
            {
                fmt::println(stderr, "Error: cluster id out of range");
                assert(0);
            }
            clusters[cluster_ids[i]].push_back(i);
        }
        #ifdef DEBUG
        this->print_clusters(clusters);
        #endif // DEBUG
        return clusters;
    }


};