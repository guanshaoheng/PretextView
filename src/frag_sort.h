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


#ifndef AISORT_H
#define AISORT_H

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include "copy_texture.h"
#include "auto_curation_state.h" 


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
    std::unordered_set<u32> excluded_fragment_idx;
    Frag4compress* frags;

    LikelihoodTable(
        Frag4compress* frags, 
        const Matrix3D<f32>* compressed_hic, 
        const f32 threshold=10.f/32769.f, 
        const std::vector<s32>& exclude_tag_idx=std::vector<s32>(),
        const u32 num_pixels_1d=32768) 
        :num_frags(frags->num), frags(frags), data(nullptr)
    {   
        auto is_exclude_tag = [&](u32 idx) -> bool
        {   
            if ((f32)frags->length[idx]/(f32)num_pixels_1d <= threshold) 
            {
                this->excluded_fragment_idx.insert(idx);
                return true;
            }
            if (exclude_tag_idx.size()==0) return false;
            for (auto i : exclude_tag_idx)
            {   
                if (i < 0) continue;
                if (frags->metaDataFlags[idx] & (1<<i)) 
                {
                    this->excluded_fragment_idx.insert(idx);
                    return true;
                }
            }
            return false;
        };

        data = new f32[num_frags * num_frags * 4];
        for (u32 i = 0; i<num_frags * num_frags * 4; i++) data[i] = -1.f;
        for (u32 i = 0; i<num_frags; i++)
        {   
            if ( is_exclude_tag(i) ) continue;
            for (u32 j = 0; j < num_frags; j++)
            {   
                if ( is_exclude_tag(j)) continue;
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
        return data[ ((i * num_frags) << 2) + (j << 2) + k];
    }

    const f32& operator() (const u32 i, const u32 j, const u32 k) const
    {
        if (i >= num_frags || j >= num_frags || k >= 4) 
        {
            std::cerr << "Error: index out of range" << std::endl;
            assert(0);
        }
        return data[ ((i * num_frags) << 2) + (j << 2) + k];
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


class FragSortTool
{
public:
    FragSortTool();

    ~FragSortTool();

    void cal_curated_fragments_order(
        const GraphData* graph, 
        FragsOrder& frags_order);

    void sort_according_likelihood_dfs(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr) const;

    void sort_according_likelihood_unionFind(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr) const;

    void sort_according_likelihood_unionFind_doFuse(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr,
        const bool doStageOne=true,
        const bool doStageTwo=false) const;
};



#endif // AISORT_H