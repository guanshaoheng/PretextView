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
#include <array>
#include <deque>
#include <functional>
#include "copy_texture.h"
#include "auto_curation_state.h"
#include "yahs_sort.h"
#include "likelihood_table.h"
#include "frags_order.h"

struct Link {
    f32 score;
    u32 frag_a;
    u32 frag_b;
    u08 is_a_head_linked;
    u08 is_b_head_linked;
    Link(f32 score_, u32 frag_a_, u32 frag_b_, u08 is_a_head_linked_, u08 is_b_head_linked_)
        :score(score_), frag_a(frag_a_), frag_b(frag_b_), is_a_head_linked(is_a_head_linked_), is_b_head_linked(is_b_head_linked_) {}
};


class FragSortTool
{
public:

    f32 threshold_ratio = 0.85;

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
        SelectArea& select_area,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr, 
        const bool sort_according_len_flag=true) const;

    void sort_according_likelihood_unionFind_doFuse(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        SelectArea& select_area,
        const f32 threshold=-0.001, 
        const Frag4compress* frags=nullptr,
        const bool doStageOne=true,
        const bool doStageTwo=false, 
        const bool sort_according_len_flag=true) const;

    void sort_according_yahs(
        const LikelihoodTable& likelihood_table,
        FragsOrder& frags_order,
        SelectArea& select_area,
        const f32 threshold=-0.001,
        const Frag4compress* frags=nullptr) const;


    void sort_method_mask(
        const LikelihoodTable& likelihood_table, 
        FragsOrder& frags_order,
        SelectArea& selected_area,
        const AutoCurationState& auto_curation_state,
        const Frag4compress* frags=nullptr, 
        const bool sort_according_len_flag=true
    ) const
    {   
        if (auto_curation_state.sort_mode == 0 || !selected_area.select_flag) // sort with union find if sorting the whole genome
        {
            this->sort_according_likelihood_unionFind( 
                likelihood_table, 
                frags_order, 
                selected_area,
                auto_curation_state.link_score_threshold, 
                frags);
        }
        else if (auto_curation_state.sort_mode == 1)
        {
            this->sort_according_likelihood_unionFind_doFuse( 
                likelihood_table, 
                frags_order, 
                selected_area,
                auto_curation_state.link_score_threshold, 
                frags, true, true);
        }
        else if (auto_curation_state.sort_mode == 2)
        {
            this->sort_according_likelihood_unionFind_doFuse( 
                likelihood_table, 
                frags_order, 
                selected_area,
                auto_curation_state.link_score_threshold, 
                frags, false, true);
        }
        else if (auto_curation_state.sort_mode == 3) // not finished yet
        {
            this->sort_according_yahs(
                likelihood_table,
                frags_order,
                selected_area,
                auto_curation_state.link_score_threshold,
                frags
            );
        }
        else 
        {
            fprintf(stderr, "[Pixel Sort] Error: Unknown sort mode (%d)\n", auto_curation_state.sort_mode);
            assert(0);
        }
    }
    
};



#endif // AISORT_H