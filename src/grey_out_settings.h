
#ifndef GREY_OUT_SETTINGS_H
#define GREY_OUT_SETTINGS_H

#include "genomeData.h"
#include <unordered_set>


struct GreyOutSettings
{   
    s32 n_flags = 64;
    s32 grey_out_flags[64];
    std::unordered_set<std::string> default_grey_out_tag = {"FalseDuplicate", "Primary"}; 
    std::unordered_set<std::string> paint_tags = {
        "vertPaint",   // paint vertically
        "horzPaint",   // paint horizontally
        "crossPaint",  // paint with cross
    };

    GreyOutSettings() 
    {
        for (u32 i = 0; i < n_flags; i ++ )
        {
            grey_out_flags[i] = 0;
        }
    }

    GreyOutSettings(meta_data* Meta_Data) 
    {
        for (u32 i = 0; i < n_flags; i ++ )
        {   
            if (default_grey_out_tag.find((char*)Meta_Data->tags[i]) != default_grey_out_tag.end())
            {
                grey_out_flags[i] = 1;
            }
            else
            {
                grey_out_flags[i] = 0;
            }
        }
    }

    void toggle_grey_out(u32 index)
    {
        if (index < n_flags) {
            grey_out_flags[index] = !grey_out_flags[index];
        }
    }

    void clear_all()
    {
        std::fill(grey_out_flags, grey_out_flags+n_flags, 0);
    }

    /* 
    Return the tag name if the tag is greyed out, otherwise return an empty string
    */
    std::string is_grey_out(u64* meta_data_flags, meta_data* Meta_Data)
    {   
        for (s32 i = 0; i  < ArrayCount(Meta_Data->tags) ; i ++ )
        {
            if (grey_out_flags[i] && (*meta_data_flags & ((u64)1 << i)))
            {
                return (char*)(Meta_Data->tags[i]);
            }
        }
        return "";
    }

    /* return:  
        0: no grey out
        1: vertical grey out
        2: horizontal grey out 
        3: corss grey out
    */
    u32 is_vert_horiz_grey_out(u64* meta_data_flags, meta_data* Meta_Data){
        for (s32 i = 0; i  < ArrayCount(Meta_Data->tags) ; i ++ ) {
            if ((*meta_data_flags & ((u64)1 << i))  ){
                if ( std::string((char*) Meta_Data->tags[i])=="vertPaint") return 1;
                else if ( std::string((char*) Meta_Data->tags[i])=="horzPaint") return 2;
                else if ( std::string((char*) Meta_Data->tags[i]) == "crossPaint") return 3;
            }
        }
        return 0;
    }

    
};



#endif // GREY_OUT_SETTINGS_H