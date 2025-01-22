


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
    std::vector<u32> selected_frag_ids;
};


struct AutoCurationState
{

private:
    s32 start_pixel = -1; // select contigs for sort
    s32 end_pixel = -1;   // select contigs for sort

public:
    u08 selected_or_exclude = 0; // 0 for select, 1 for exclude
    u08 select_mode = 0; // 0: deactive, 1: set mode, 2: delete mode from start, 3: delete mode from end
    // Variables to store the current values
    u32 smallest_frag_size_in_pixel = 2;
    f32 link_score_threshold = 0.4f;

    // Variables for the editing UI state
    bool show_yahs_erase_confirm_popup = false;
    bool show_yahs_redo_confirm_popup = false;
    //sorting mode
    u32 sort_mode = 0; // 0: union find, 1: fuse union find, 2 deep fuse
    std::vector<std::string> sort_mode_names = {"Union Find", "Fuse Union Find", "Deep Fuse"};
    u08 frag_size_buf[16];
    u08 score_threshold_buf[16];
    AutoCurationState()
    {
        set_buf();
    }

    void set_buf()
    {
        memset(frag_size_buf, 0, 16);
        memset(score_threshold_buf, 0, 16);
        snprintf((char*)frag_size_buf, sizeof(frag_size_buf), "%u", smallest_frag_size_in_pixel);
        memset(score_threshold_buf, 0, sizeof(score_threshold_buf));
        snprintf((char*)score_threshold_buf, sizeof(score_threshold_buf), "%.3f", link_score_threshold);
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

    void get_selected_fragments(std::vector<u32>& frag_ids, map_state* Map_State, u32 number_of_pixels_1D, u08 exclude_flag=false)
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

        if (!frag_ids.empty() )
        {
            frag_ids.clear();
        }
        
        u32 last_contig_id = Map_State->contigIds[this->start_pixel];
        frag_ids.push_back(last_contig_id);
        for (u32 i = start_pixel+1; i <= this->end_pixel; i ++ )
        {
            if (last_contig_id == Map_State->contigIds[i]) continue;
            else
            {   
                last_contig_id = Map_State->contigIds[i];
                frag_ids.push_back(last_contig_id);
            }
        }
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