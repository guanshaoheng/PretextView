#ifndef LIKELIHOOD_TABLE_H
#define LIKELIHOOD_TABLE_H

#include "utilsPretextView.h"
#include "copy_texture.h"
#include <fmt/core.h>
#include <cstdio>
struct LikelihoodTable
{
public:
    f32* data = nullptr;       // with shape [num_frags, num_frags, 4]
    u32 num_frags;   // all the frags including the filtered out ones
    u32 size ;
    std::unordered_set<u32> excluded_fragment_idx;
    Frag4compress* frags = nullptr;

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
        size = num_frags * num_frags * 4;
        data = new f32[size];
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

    const void output_fragsInfo_likelihoodTable(const std::string& file_name, const Matrix3D<f32>* compressed_hic) const
    {       
        FILE* ofs = fopen(file_name.c_str(), "w");
        if (!ofs)
        {
            fmt::print("Error: failed to open: {}", file_name) ;
            assert(0);
        }

        fmt::print( ofs, "# frags num: {} \n", num_frags);
        fmt::print( ofs, "# frags_index\tlength\trepeat\tcoverage\n");
        for (u32 i = 0; i < num_frags; i++)
        {
            fmt::print(
                ofs, 
                "{}\t{}\t{}\t{}\t{}\n", 
                i, 
                frags->length[i], 
                0, 
                0, 
                0);
        }
        fmt::print(ofs, "\n");

        // likelihood table
        fmt::print(ofs, "# likelihood table\n");
        compressed_hic->output_to_file(ofs);

        fclose(ofs);

        fmt::print("[DEBUG_OUTPUT_LIKELIHOOD_TABLE]: Likelihood table saved to: {}\n", file_name);
    }
};



#endif // LIKELIHOOD_TABLE_H