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

#ifndef COPY_TEXTURE_H
#define COPY_TEXTURE_H
#include <iostream>
#include <utility>
#include <fstream>
#include <random>

#include "utilsPretextView.h"
#include "genomeData.h"
#include "auto_curation_state.h"

struct Frag4compress {
    u32 num;
    u32* frag_id;
    u32* startCoord; // global start coordinate
    u32* length;
    bool* inversed;
    u32 total_length;
    u64* metaDataFlags;

    Frag4compress(const contigs* Contigs)
        : total_length(0)
    {
        re_allocate_mem(Contigs);
    }

    ~Frag4compress()
    {
        cleanup();
    }

    void re_allocate_mem(
        const contigs* Contigs, 
        const SelectArea* select_area=nullptr)
    {
        cleanup();
        u08 using_select_area = (select_area != nullptr && select_area->select_flag)?1:0;
        num = Contigs->numberOfContigs;
        if (using_select_area)
        {
            num = select_area->selected_frag_ids.size();
            if (num < 2)
            {
                fprintf(stderr, "The number_of_select_fragments_for_sorting(%d) should not be less than 2, file:%s (line:%d)\n", num, __FILE__, __LINE__);
                std::abort();
            }
        }
        frag_id = new u32[num];
        startCoord = new u32[num]; // TODO (shaoheng): it is wired that if I use std::vector<u32> here, the debugger will be stuck where I call the constructor. Sometimes it can work with std::vector, but sometimes it cannot.
        length = new u32[num];
        inversed = new bool[num];
        metaDataFlags = new u64[num];
        total_length = 0;

        // Initialize the startCoord, length, and inversed
        inversed[0] = false;
        if (using_select_area)
        {   
            frag_id[0] = select_area->selected_frag_ids[0];
            startCoord[0] =select_area->start_pixel;
            length[0] = Contigs->contigs[select_area->selected_frag_ids[0]].length;
        }
        else
        {
            frag_id[0] = 0;
            startCoord[0] = 0;
            length[0] = Contigs->contigs[0].length;
        }
        total_length = length[0];
        for (u32 i = 1; i < num; i++)
        {   
            u32 contig_id = using_select_area?select_area->selected_frag_ids[i]:i;
            frag_id[i] = contig_id;
            inversed[i] = false; // todo (shaoheng) add the inversed information
            startCoord[i] = startCoord[i-1] + length[i-1];
            length[i] = Contigs->contigs[contig_id].length;
            metaDataFlags[i] = (Contigs->contigs[contig_id].metaDataFlags == nullptr)?0:*(Contigs->contigs[contig_id].metaDataFlags);
            total_length += length[i];
        }
    }

private:
    void cleanup()
    {   
        if (frag_id)
        {
            delete[] frag_id;
            frag_id = nullptr;
        }

        if (startCoord)
        {
            delete[] startCoord;
            startCoord = nullptr;
        }
        if (length)
        {
            delete[] length;
            length = nullptr;
        }
        if (inversed)
        {
            delete[] inversed;
            inversed = nullptr;
        }
        if (metaDataFlags)
        {
            delete[] metaDataFlags;
            metaDataFlags = nullptr;
        }
        total_length = 0;
    }
};


void scroll_callback(GLFWwindow* window, f64 xoffset, f64 yoffset);

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);


struct TranslationOffset
{
    f32 x;
    f32 y;
    TranslationOffset() : x(0.0f), y(0.0f) {}
};


struct MassCentre
{
    f32 row;
    f32 col;
    MassCentre() : row(0.0f), col(0.0f) {}
};


struct Show_State
{   
public:
    f32 zoomlevel;
    bool isDragging;
    f64 lastMouseX;
    f64 lastMouseY;
    TranslationOffset translationOffset;
    bool show_menu_window;
    Show_State();
    ~Show_State();
};


void get_linear_mask(f32* linear_array, u32 length);


template<typename T>
class Matrix2D
{
private:
    T* data;
public:
    u32 row_num, col_num, length;
    u32 shape[2];
    Matrix2D(u32 row_num_, u32 col_num_)
        : row_num(row_num_), col_num(col_num_)
    {   
        shape[0] = row_num_;
        shape[1] = col_num_;
        if (row_num_ <= 0 || col_num_ <= 0)
        {
            fprintf(stderr, "The row_num(%d), col_num(%d) should be larger than 0\n", row_num_, col_num_);
            assert(0);
        }
        length = row_num * col_num;
        data = new T[length];
        memset(data, 0, length * sizeof(T));
    }

    ~Matrix2D()
    {
        if (data)
        {
            delete[] data;
            data = nullptr;
        }
    }

    void check_indexing(const u32& row, const u32& col) const
    {
        if (row >= row_num || col>= col_num) 
        {
            fprintf(stderr, "Index [%d, %d]  is out of the maximum [%d, %d]\n", row, col, row_num-1, col_num-1);
            assert(0);
        }
        return ;
    }

    // Access operator
    T& operator()(const u32& row, const u32& col) 
    {
        check_indexing(row, col);
        return data[row * col_num + col];
    }

    const T& operator()(const u32& row, const u32& col) const 
    {
        check_indexing(row, col);
        return data[row * col_num + col];
    }

    T* get_data_ptr() const
    {
        return data;
    }

};



template<typename T>
class Matrix3D
{
private:
    T* data;
public:
    u32 row_num, col_num, layer_num, length;
    u32 shape[3];
    Matrix3D(
        u32 row_num_, 
        u32 col_num_, 
        u32 layer_)
    {   
        MY_CHECK(0);
        re_allocate_mem(row_num_, col_num_, layer_);
    }

    ~Matrix3D()
    {
        cleanup();
    }

    void cleanup()
    {   

        MY_CHECK(0);
        if (data)
        {   
            MY_CHECK(0);
            delete[] data;
            data = nullptr;
        }
    }

    void re_allocate_mem(u32 row_num_, u32 col_num_, u32 layer_)
    {   
        MY_CHECK(0);
        cleanup();
        row_num = row_num_;
        col_num = col_num_;
        layer_num = layer_;
        shape[0] = row_num_;
        shape[1] = col_num_;
        shape[2] = layer_;
        length = row_num * col_num * layer_num;

        MY_CHECK(0);
        data = new T[length];

        MY_CHECK(0);
        memset(data, 0, length * sizeof(T));
        MY_CHECK(0);
    }

    void check_indexing(const u32& row, const u32& col, const u32& layer) const
    {
        if (row >= row_num || col>= col_num || layer >= layer_num) 
        {
            fprintf(stderr, "Index [%d, %d, %d]  is out of the maximum [%d, %d, %d]\n", row, col, layer, row_num-1, col_num-1, layer_num-1);
            assert(0);
        }
        return ;
    }

     // Access operator
    T& operator()(const u32& row, const u32& col, const u32& layer) {
        check_indexing(row, col, layer);
        return data[row * col_num * layer_num + col * layer_num + layer];
    }

    const T& operator()(const u32& row, const u32& col, const u32& layer) const {
        check_indexing(row, col, layer);
        return data[row * col_num * layer_num + col * layer_num + layer];
    }

    void set_one(const u32& row, const u32& col, const u32& layer, const T& value)
    {
        check_indexing(row, col, layer);
        data[row * col_num * layer_num + col* layer_num + layer] = value;
    }
};


struct CompressedExtensions
{   
    u32 num;           // number of extensions: 2 : coverage, repeat_density
    u32 num_frags;
    char** names;
    bool* exist_flags;
    f32** data;        //  coverage, repeat_density
    CompressedExtensions(u32 num_frags_)
    {     
        re_allocate_mem(num_frags_); 
    }

    void re_allocate_mem(u32 num_frags_)
    {   
        cleanup();
        try
        {   
            num_frags = num_frags_;
            num = 2;

            names = new char*[num];
            names[0] = new char[10];
            strcpy(names[0], "coverage");
            names[1] = new char[15];
            strcpy(names[1], "repeat_density");
            exist_flags = new bool[num];
            for (u32 i = 0; i < num; i++)
            {
                exist_flags[i] = false;
            }
            data = new f32*[num];
            for (u32 i = 0; i < num; i++)
            {
                data[i] = new f32[num_frags];
                for (u32 j = 0; j < num_frags; j++)
                {
                    data[i][j] = 0.f;
                }
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            cleanup();
            assert(0);
        }
    }

    ~CompressedExtensions()
    {
        cleanup();
    }

private:
    void cleanup()
    {
        if (names)
        {   
            for (u32 i = 0; i < num; i++)
            {
                if (names[i])
                {
                    delete[] names[i];
                    names[i] = nullptr;
                }
            }
        }
        if (exist_flags)
        {
            delete[] exist_flags;
            exist_flags = nullptr;
        }
        if (data)
        {
            for (u32 i = 0; i < num; i++)
            {
                if (data[i])
                {
                    delete[] data[i];
                    data[i] = nullptr;
                }
            }
            delete[] data;
            data = nullptr;
        }
    }
};


inline // c++17 needed
std::unordered_map<u32, u32> 
Switch_Channel_Symetric = 
{   
    /*
                    0 - 1
         (i)        |   |  
                    3 - 2

        0 - 3
        |   |        (j)
        1 - 2
    */
    {0, 0},
    {1, 3},
    {2, 2},
    {3, 1}
};


struct Values_on_Channel
{
    f32 c[4];
    Values_on_Channel()
    {
        initilize();
    }
    void initilize()
    {
        c[0] = 0.f;
        c[1] = 0.f;
        c[2] = 0.f;
        c[3] = 0.f;
    }
}; 


struct Sum_and_Number 
{
    u64 sum;
    u32 number;
};
    


class GraphData
{
private:
    u32 num_nodes;             // num of selected nodes
    u32* selected_node_idx;    // the selected node global(original) index
    u32 num_edges;             // selected num of edges
    u32 num_node_properties;   // 1 + 1 + 1 ...
    u32 num_edge_properties;   // 4 + 1 + 2 ...
    u32 num_full_edges;        // num_nodes * (num_nodes - 1) / 2
public:
    Frag4compress* frags;
    Matrix2D<f32> nodes;              // [num_nodes, num_node_properties]
    Matrix2D<u32> edges_index;        // [2, num_edges]
    Matrix2D<f32> edges;              // [num_edges, num_edge_properties]
    Matrix2D<u32> edges_index_full;   // [2, num_full_edges]
    
    GraphData(
        u32 num_nodes_, 
        u32 num_edges_, 
        u32 num_node_properties_, 
        u32 num_edge_properties_, 
        const std::vector<u32>& selected_node_idx_,
        Frag4compress* frags_)
        : num_nodes(num_nodes_), num_edges(num_edges_), num_node_properties(num_node_properties_), num_edge_properties(num_edge_properties_), 
        frags(frags_),
        nodes(num_nodes_, num_node_properties_), 
        edges(num_edges_, num_edge_properties_), 
        edges_index(2, num_edges_), 
        num_full_edges((num_nodes_ * (num_nodes_ - 1)) >> 1), 
        edges_index_full(2, num_full_edges)
    {   
        if (num_nodes <= 0 || num_edges <= 0 || num_node_properties <= 0 || num_edge_properties <= 0)
        {
            fprintf(stderr, "The num_nodes(%d), num_edges(%d), num_node_properties(%d), num_edge_properties(%d) should be larger than 0\n", num_nodes, num_edges, num_node_properties, num_edge_properties);
            assert(0);
        }

        selected_node_idx = new u32[num_nodes];
        for (u32 i = 0; i < num_nodes; i++) selected_node_idx[i] = selected_node_idx_[i];

        u32 cnt = 0 ;
        for (u32 i = 0 ; i < num_nodes; i++)
        {
            for (u32 j = i+1; j < num_nodes; j++)
            {
                edges_index_full(0, cnt) = i;
                edges_index_full(1, cnt) = j;
                cnt++;
            }
        }
        if (cnt != num_full_edges)
        {
            fprintf(stderr, "The cnt(%d) != num_full_edges(%d)\n", cnt, num_full_edges);
            assert(0);
        }
    }

    ~GraphData()
    {
        if (selected_node_idx)
        {
            delete[] selected_node_idx;
            selected_node_idx = nullptr;
        }
    }

    u32 get_num_nodes() const
    {
        return num_nodes;
    }

    u32 get_num_edges() const
    {
        return num_edges;
    }

    u32 get_num_node_properties() const
    {
        return num_node_properties;
    }

    u32 get_num_edge_properties() const
    {
        return num_edge_properties;
    }

    u32 get_num_full_edges() const
    {
        return num_full_edges;
    }

    const f32* get_nodes_data_ptr() const
    {
        return nodes.get_data_ptr();
    }

    const u32* get_edge_index_data_ptr() const
    {
        return edges_index.get_data_ptr();
    }

    const f32* get_edges_data_ptr() const
    {
        return edges.get_data_ptr();
    }

    const u32* get_edges_index_full_data_ptr() const
    {
        return edges_index_full.get_data_ptr();
    }

    const u32* get_selected_node_idx() const
    {
        return selected_node_idx;
    }

};



class TexturesArray4AI
{
private:
    char name[256];
    GLuint shaderProgram, vao, vbo, ebo;
    bool hic_shader_initilised=false;
    u32 num_textures_1d;
    u32 total_num_textures;
    u32 texture_resolution; 
    u32 texture_resolution_exp;
    u32 num_pixels_1d;
    Frag4compress* frags = nullptr;
    Matrix3D<f32>* compressed_hic = nullptr; // [0 - 3] channel represent the 4 directions, channel 4 represents the average of the interaction block
    CompressedExtensions* compressed_extensions = nullptr;
    MassCentre* mass_centres = nullptr;
    unsigned char** textures=nullptr;
    bool is_copied_from_buffer;
    bool is_compressed;
public:
    TexturesArray4AI(u32 num_textures_1d, u32 texture_resolution, char* fileName, const contigs* Contigs);

    ~TexturesArray4AI();

    u08 operator()(u32 row, u32 column) const;

    f32 get_textures_diag_mean(int shift=0);

    void copy_textures_to_matrix();

    Frag4compress* get_frags() const
    {   
        if (frags == nullptr)
        {
            std::cerr << "The frags is not initialized" << std::endl;
            assert(0);
        }
        return frags;
    }

    const Matrix3D<f32> *get_compressed_hic() const
    {   
        if (compressed_hic == nullptr)
        {
            std::cerr << "The compressed_hic is not initialized" << std::endl;
            assert(0);
        }
        return compressed_hic;
    }

    u32 get_num_frags() const // total number of fragments
    {
        if (frags == nullptr)
        {
            std::cerr << "The frags is not initialized" << std::endl;
            assert(0);
        }
        return frags->num;
    }

    u32 get_total_num_textures(){return total_num_textures;}

    u32 get_texture_resolution(){return texture_resolution;}

    u32 get_num_textures_1d(){return num_textures_1d;}

    void check_copied_from_buffer() const;
    void copy_buffer_to_textures(const contact_matrix *contact_matrix_, bool show_flag=false);
    void copy_buffer_to_textures_dynamic(const contact_matrix *contact_matrix_, bool show_flag=false);

    f32 cal_diagonal_mean_within_fragments(int shift, const contigs* Contigs);

    void cal_compressed_hic(
        const contigs* Contigs, 
        const extension_sentinel& Extensions,
        bool is_extension_required=true, 
        bool is_massCenter_required=true,
        const SelectArea* select_area=nullptr,
        f32 D_hic_ratio=0.05f, 
        u32 maximum_D=5000, 
        f32 min_hic_density = 30.f);
    void cal_compressed_extension(const extension_sentinel &Extensions);

    Sum_and_Number get_fragement_diag_mean_square(
        u32 offset,
        u32 length,
        int shift);

    Sum_and_Number get_interaction_block_diag_sum_number(
        u32 offset_row, 
        u32 offset_column, 
        u32 num_row, 
        u32 num_column, 
        int shift,         // distance from the angle of the block [0 ~ min(num_row,num_column)], 0 means no shift, thus the minimum distance is 1
        int channel);

    f32 cal_block_mean(
        const u32& offset_row, 
        const u32& offset_column, 
        const u32& num_row, 
        const u32& num_column) const;
    
    void cal_mass_centre(
        const u32& offset_row, 
        const u32& offset_column, 
        const u32& num_row, 
        const u32& num_column,
        MassCentre* mass_centre);

    void get_interaction_score(
        const u32& row, 
        const u32& column, 
        const u32& num_row, 
        const u32& num_column, 
        const u32& D,
        const std::vector<f32>& norm_diag_mean,
        Values_on_Channel& buffer_values_on_channel);
    
    void cal_graphData(
        GraphData*& graph_data, 
        f32 selected_edges_ratio=0.5f, 
        f32 random_selected_edges_ratio = 0.015f, 
        u32 minimum_edges=8u, 
        u32 minimum_random_edges=4u, 
        f32 length_threshold=10.f / (32768.f + 1.f), 
        u32 num_node_properties=8, 
        u32 num_edge_properties=7
    );

    void prepare_tmp_texture2d_array(GLuint& tmp_texture2d_array);
    void show_collected_texture();
    void show_collected_textures(); // after getting the textures, show them on the screen for validation
    
    void output_textures_to_file() const;
    void output_compressed_hic_to_file() const;
    void output_compressed_extension_to_file() const;
    void output_mass_centres_to_file() const;
    void output_frags4AI_to_file() const;
    void output_compressed_hic_massCetre_extension_for_python() const;
};




#endif // COPY_TEXTURE_H