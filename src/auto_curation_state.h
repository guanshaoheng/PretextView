


#ifndef AUTO_CURATION_STATE_H 
#define AUTO_CURATION_STATE_H

#include "utilsPretextView.h"
#include "showWindowData.h"
#include "genomeData.h"


struct SelectArea
{   
public:
    SelectArea() {};
    u08 select_flag = 0; // no selected area set
    u32 start_pixel = 0;
    u32 end_pixel = 0;
    s32 source_frag_id = -1;
    s32 sink_frag_id = -1;
    std::vector<u32> selected_frag_ids;


    void clearup()
    {
        this->select_flag = 0;
        this->start_pixel = 0;
        this->end_pixel = 0;
        this->source_frag_id = -1;
        this->sink_frag_id = -1;
        this->selected_frag_ids.clear();
    }

    /*
    first frag id, including the source fragment
    */
    u32 get_first_frag_id()
    {
        return this->source_frag_id >=0? this->source_frag_id : this->selected_frag_ids[0];
    }

    /*
    len in pixel, including the source and sink fragments
    */
    u32 get_selected_len(const contigs* Contigs) const
    {
        u32 len = 0;
        if (this->source_frag_id>=0)
        {
            len += Contigs->contigs_arr[this->source_frag_id].length;
        }
        for (const auto& frag_id : this->selected_frag_ids)
        {
            len += Contigs->contigs_arr[frag_id].length;
        }
        if (this->sink_frag_id>=0)
        {
            len += Contigs->contigs_arr[this->sink_frag_id].length;
        }
        return len;
    }

    /*
    number of fragments to be sorted, including the source and sink fragments
    */
    u32 get_to_sort_frags_num()
    {
        return this->selected_frag_ids.size() + (this->source_frag_id>=0) + (this->sink_frag_id>=0);
    }

    /*
    id of fragments to be sorted, including the source and sink fragments
    */
    std::vector<u32> get_to_sort_frags_id(const contigs* Contigs)
    {
        std::vector<u32> to_sort_frags;
        if (this->source_frag_id>=0)
        {
            to_sort_frags.push_back(this->source_frag_id);
        }
        for (const auto& frag_id : this->selected_frag_ids)
        {
            to_sort_frags.push_back(frag_id);
        }
        if (this->sink_frag_id>=0)
        {
            to_sort_frags.push_back(this->sink_frag_id);
        }
        return to_sort_frags;
    }
};


struct AutoCurationState
{

private:
    s32 start_pixel = -1; // select contigs for sort
    s32 end_pixel = -1;   // select contigs for sort

public:
    f32 mask_color[4] = {0.906f, 0.03921f, 0.949f, 0.2f}; // used to show the selected area.
    f32 mask_color_default[4] = {0.906f, 0.03921f, 0.949f, 0.2f}; // default color
    u08 selected_or_exclude = 0; // 0 for select, 1 for exclude
    u08 select_mode = 0; // 0: deactive, 1: set mode, 2: delete mode from start, 3: delete mode from end
    // Variables to store the current values
    u32 smallest_frag_size_in_pixel = 2;
    f32 link_score_threshold = 0.4f;

    f32 auto_cut_threshold = 0.05f;
    u32 auto_cut_diag_window_for_pixel_mean= 8;
    u32 auto_cut_smallest_frag_size_in_pixel = 8;

    // Variables for the editing UI state
    bool show_autoSort_erase_confirm_popup = false;
    bool show_autoSort_redo_confirm_popup = false;
    //sorting mode
    u32 sort_mode = 0; // 0: union find, 1: fuse union find, 2 deep fuse
    std::vector<std::string> sort_mode_names = {"Union Find", "Fuse Union Find", "Deep Fuse"};
    
    // auto sort
    char frag_size_buf[16];
    char score_threshold_buf[16];

    // auto cut
    char auto_cut_threshold_buf[16];
    char auto_cut_diag_window_for_pixel_mean_buf[16];
    char auto_cut_smallest_frag_size_in_pixel_buf[16];

    AutoCurationState()
    {
        set_buf();
    }
    
    void set_buf()
    {   
        // sort: smallest_frag_size
        memset(this->frag_size_buf, 0, sizeof(frag_size_buf));
        snprintf((char*)frag_size_buf, sizeof(frag_size_buf), "%u", this->smallest_frag_size_in_pixel);
        // sort: link_score_threshold
        memset(score_threshold_buf, 0, sizeof(score_threshold_buf));
        snprintf((char*)this->score_threshold_buf, sizeof(this->score_threshold_buf), "%.3f", this->link_score_threshold);

        // // cut: auto_cut_threshold
        // memset(this->auto_cut_threshold_buf, 0, sizeof(this->auto_cut_threshold_buf));
        // snprintf((char*)this->auto_cut_threshold_buf, sizeof(this->auto_cut_threshold_buf), "%.3f", this->auto_cut_threshold);
        // // cut: auto_cut_diag_window_for_pixel_mean
        // memset(this->auto_cut_diag_window_for_pixel_mean_buf, 0, sizeof(this->auto_cut_diag_window_for_pixel_mean_buf));
        // snprintf((char*)this->auto_cut_diag_window_for_pixel_mean_buf, sizeof(this->auto_cut_diag_window_for_pixel_mean_buf), "%u", this->auto_cut_diag_window_for_pixel_mean);
        // // cut: auto_cut_smallest_frag_size_in_pixel
        // memset(this->auto_cut_smallest_frag_size_in_pixel_buf, 0, sizeof(this->auto_cut_smallest_frag_size_in_pixel_buf));
        // snprintf((char*)this->auto_cut_smallest_frag_size_in_pixel_buf, sizeof(this->auto_cut_smallest_frag_size_in_pixel_buf), "%u", this->auto_cut_smallest_frag_size_in_pixel);


        fmt::format_to((char*)auto_cut_threshold_buf, "{:.3f}", this->auto_cut_threshold);
        fmt::format_to((char*)auto_cut_diag_window_for_pixel_mean_buf, "{}", this->auto_cut_diag_window_for_pixel_mean);
        fmt::format_to((char*)auto_cut_smallest_frag_size_in_pixel_buf, "{}", this->auto_cut_smallest_frag_size_in_pixel);
    }

    void set_mask_color(f32* color)
    {
        for (u32 i = 0; i < 4; i++)
        {
            this->mask_color[i] = color[i];
        }
    }

    void update_value_from_buf()
    {   
        // sort 
        this->smallest_frag_size_in_pixel = (u32)atoi((char*)this->frag_size_buf);
        this->link_score_threshold = (float)atof((char*)this->score_threshold_buf);
        if (this->link_score_threshold > 1.0f || this->link_score_threshold < 0.0f) 
        {   
            printf("[Auto Sort] Warning: link Score Threshold should be in the range of [0, 1]\n");
            this->link_score_threshold = std::max(0.0f, std::min(1.0f, this->link_score_threshold));
        }

        // cut
        this->auto_cut_threshold = (f32)atof((char*)this->auto_cut_threshold_buf);
        this->auto_cut_diag_window_for_pixel_mean = (u32)atoi((char*)this->auto_cut_diag_window_for_pixel_mean_buf);
        this->auto_cut_smallest_frag_size_in_pixel = (u32)atoi((char*)this->auto_cut_smallest_frag_size_in_pixel_buf);

        this->set_buf();
    }

    std::string get_sort_mode_name() const
    {
        return sort_mode_names[sort_mode];
    }

    void clear() 
    {
        this->start_pixel = -1;
        this->end_pixel = -1;
    }

    void set_start(u32 start_pix) 
    {
        this->start_pixel = start_pix;
        this->select_mode = 2;
    }

    void set_end(u32 end_pix)
    {
        this->end_pixel = end_pix;
        this->select_mode = 3;
    }

    s32 get_start()
    {   
        return this->start_pixel;
    }

    s32 get_end()
    {
        return this->end_pixel;
    }

    u32 get_selected_fragments_num(
        map_state* Map_State,
        u32 number_of_pixels_1D
    ) 
    {
        if (this->start_pixel > number_of_pixels_1D || this->end_pixel > number_of_pixels_1D || this->start_pixel < 0 || this->end_pixel < 0)
        {
            return 0;
        }
        if (this->start_pixel > this->end_pixel)
        {
            std::swap(this->start_pixel, this->end_pixel);
        }

        u32 num_selected_frags = 1;
        for (u32 i = this->start_pixel+1; i <= this->end_pixel; i++)
        {
            if (Map_State->contigIds[i] != Map_State->contigIds[i-1])
            {
                num_selected_frags ++;
            }
        }
        return num_selected_frags;
    }

    std::vector<u32> get_selected_fragments_id(
        map_state* Map_State,
        u32 number_of_pixels_1D
    )
    {
        std::vector<u32> selected_frag_ids={};
        if (this->start_pixel > number_of_pixels_1D || this->end_pixel > number_of_pixels_1D || this->start_pixel < 0 || this->end_pixel < 0)
        {
            return selected_frag_ids;
        }
        if (this->start_pixel > this->end_pixel)
        {
            std::swap(this->start_pixel, this->end_pixel);
        }

        u32 last_contig_id = Map_State->contigIds[this->start_pixel];
        selected_frag_ids.push_back(last_contig_id);
        for (u32 i = this->start_pixel+1; i <= this->end_pixel; i++)
        {
            if (last_contig_id == Map_State->contigIds[i]) continue;
            else
            {
                last_contig_id = Map_State->contigIds[i];
                selected_frag_ids.push_back(last_contig_id);
            }
        }
        return selected_frag_ids;
    }


    void get_selected_fragments(
        SelectArea& select_area, 
        map_state* Map_State, 
        u32 number_of_pixels_1D, 
        contigs* Contigs,
        u08 exclude_flag=false)
    {   
        if (this->start_pixel > number_of_pixels_1D || this->end_pixel > number_of_pixels_1D || this->start_pixel < 0 || this->end_pixel < 0)
        {   
            // char buff[128];
            // snprintf((char*)buff, 128, "start_pixel(%d) and end_pixel(%d) should be within [0, Number_of_Pixels_1D(%d)-1]", this->start_pixel, this->end_pixel, number_of_pixels_1D);
            // MY_CHECK((const char*) buff);
            return ;
        }
        if (this->start_pixel > this->end_pixel)
        {
            std::swap(this->start_pixel, this->end_pixel);
        }

        if (!select_area.selected_frag_ids.empty() )
        {
            select_area.clearup();
        }
        
        // define source frag
        if (this->start_pixel>0)
        {   
            u32 frag_id = Map_State->contigIds[this->start_pixel-1];
            if (Contigs->contigs_arr[frag_id].length >= this->smallest_frag_size_in_pixel)
            {
                select_area.source_frag_id = frag_id;
            }
            else
            {   
                select_area.source_frag_id = -1;
                fmt::print("The source_frag_len ({}) < smallest_length ({}), not set the source frag.\n", Contigs->contigs_arr[frag_id].length, this->smallest_frag_size_in_pixel);
            }
        }
        
        // push the selected frag id into select_area
        select_area.start_pixel = this->start_pixel;
        select_area.end_pixel = this->end_pixel;
        u32 last_contig_id = Map_State->contigIds[this->start_pixel];
        select_area.selected_frag_ids.push_back(last_contig_id);
        for (u32 i = start_pixel+1; i <= this->end_pixel; i ++ )
        {
            if (last_contig_id == Map_State->contigIds[i]) continue;
            else
            {   
                last_contig_id = Map_State->contigIds[i];
                select_area.selected_frag_ids.push_back(last_contig_id);
            }
        }

        // define sink frag
        if (this->end_pixel + 1 <= number_of_pixels_1D - 1)
        {   
            u32 frag_id = Map_State->contigIds[this->end_pixel+1];
            if (Contigs->contigs_arr[frag_id].length >= this->smallest_frag_size_in_pixel)
            {
                select_area.sink_frag_id = frag_id;
            }
            else
            {   
                select_area.sink_frag_id = -1;
                fmt::print("The tail_frag_len ({}) < smallest_length ({}), not set the tail frag.\n", Contigs->contigs_arr[frag_id].length, this->smallest_frag_size_in_pixel);
            }
        }
        // set the select_area to valid
        if (!select_area.selected_frag_ids.empty()) select_area.select_flag = 1;
    }

    void update_sort_area(
        u32 x_pixel, map_state* Map_State, u32 num_pixels_1d
    )
    {   
        if (this->select_mode == 1) // expand area
        {   
            if (this->start_pixel == -1) this->start_pixel = x_pixel;
            if (this->end_pixel == -1) this->end_pixel = x_pixel;
            if (x_pixel < this->start_pixel)
            {
                this->start_pixel = x_pixel;
            }
            else if (x_pixel > this->end_pixel)
            {
                this->end_pixel = x_pixel;
            }
            else if (this->start_pixel < x_pixel && x_pixel < this->end_pixel)
            {   
                u32 dis_to_start = std::abs(this->start_pixel - (s32)x_pixel);
                u32 dis_to_end = std::abs(this->end_pixel - (s32)x_pixel);
                if (dis_to_start < dis_to_end)
                {
                    this->start_pixel = x_pixel;
                }
                else 
                {
                    this->end_pixel = x_pixel;
                }
            }
        }
        else if (this->select_mode == 2) // narrow from start
        {
            if (x_pixel > this->start_pixel && x_pixel <= this->end_pixel)
            {
                this->start_pixel = x_pixel;
            }
            else if (x_pixel> end_pixel)
            {
                clear();
            }
        }
        else if (this->select_mode == 3) // narrow from end
        {
            if (this->start_pixel <= x_pixel && x_pixel < this->end_pixel)
            {
                this->end_pixel = x_pixel;
            }
            else if (x_pixel < start_pixel)
            {
                clear();
            }
        }

        // update start_pixel and end_pixel to cover the range
        if (select_mode == 1 && this->start_pixel!=-1 && this->end_pixel!=-1)
        {
            u32 tmp_pixel = this->start_pixel;
            while ( this->start_pixel>0 && Map_State->contigIds[tmp_pixel] == Map_State->contigIds[this->start_pixel])
            {
                this->start_pixel -- ;
            }
            if (Map_State->contigIds[tmp_pixel] != Map_State->contigIds[this->start_pixel])  this->start_pixel ++ ;
            tmp_pixel = this->end_pixel;
            while ( (this->end_pixel < num_pixels_1d - 1) && Map_State->contigIds[tmp_pixel] == Map_State->contigIds[this->end_pixel])
            {
                this->end_pixel ++ ;
            }
            if (Map_State->contigIds[tmp_pixel] != Map_State->contigIds[this->end_pixel]) this->end_pixel --;
        }
        return;
    }

};

#endif // AUTO_CURATION_STATE_H 