

#ifndef parse_agp_h
#define parse_agp_h

#include <fmt/core.h>
#include <string>
#include <vector>
#include <regex>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "genomeData.h"



struct Date
{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    Date()
        : year(-1), month(-1), day(-1), hour(-1), minute(-1), second(-1) {}
    
    Date(int y, int m, int d, int h, int min, int s)
        : year(y), month(m), day(d), hour(h), minute(min), second(s) {}

    Date(int y, int m, int d)
        : year(y), month(m), day(d), hour(-1), minute(-1), second(-1) {}

    Date(const Date& other)
        : year(other.year), month(other.month), day(other.day),
          hour(other.hour), minute(other.minute), second(other.second) {}

    std::string to_string() const
    {   
        if (second>=0 && minute>=0 && hour>=0)
            return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", year, month, day, hour, minute, second);
        else if (year>=0 && month>=0 && day>=0)
            return fmt::format("{:04d}-{:02d}-{:02d}", year, month, day);
        else if (year>=0 && month>=0)
            return fmt::format("{:04d}-{:02d}", year, month);
        else if (year>=0)
            return fmt::format("{:04d}", year);
        else return "Unknown Date";
    }
};


struct Original_Contig_agp
{
    int len;              // len in bp
    int num_frags;
    std::vector<int> starts; // record the start position of each fragment

    Original_Contig_agp()
        : len(0), num_frags(0) {}
    Original_Contig_agp(int l, int n) : len(l), num_frags(n) {}
        
};

struct Scaff_agp
{
    int len;            // len in bp
    bool is_painted;
    Scaff_agp( bool p)
        : len(0), is_painted(p) {}
};


struct Frag
{
    const int orig_contig_id;   // start from 0
    const int scaff_id;         // start from 0
    const int start;            // start position (bp) in the original contig
    const int len;              // len in bp
    int local_index = -1;       // index within orignal contig
    const bool is_reverse;
    const bool is_painted ;
    const uint64_t meta_data_flag = 0;
    Frag(const int oci, const int si, int s, int l, bool r, bool p, const uint64_t& m)
        : orig_contig_id(oci), scaff_id(si), start(s), len(l), is_reverse(r), is_painted(p), meta_data_flag(m) {}
};


int get_scaff_id(const std::string& scaff_name)
{   
    std::smatch match;
    if (!std::regex_search(scaff_name, match, std::regex("^\\w+_(\\d+)$")))
    {
        fmt::print(stderr, "Error: scaff name {} not match ^\\w+_(\\d+)$ \n", scaff_name);
        assert(0);
    } else 
    {
        return std::stoi(match[1].str()) - 1; 
    }
}


class AssemblyAGP
{
public:
    std::string sample_name;
    std::string agp_version;
    std::string description;
    Date date;
    double bp_per_pixel;

    std::vector<Frag> frags;
    std::unordered_map<std::string, Original_Contig_agp> original_contigs;
    std::vector<Scaff_agp> scaffs;
    int total_bp = 0;


    int get_num_painted_scaff() const
    {
        int num = 0;
        for (const auto& scaff : scaffs)
        {
            if (scaff.is_painted) num++;
        }
        return num;
    }

    void add_frag(
        const original_contig* Original_Contigs, const int& num_original_contigs,
        const std::string& scaff_name, 
        const std::string& original_contig_name,
        const int& start_local, const int& end_local, 
        bool is_reverse,
        bool is_painted,
        const uint64_t& meta_tags )
    {   
        int original_contig_id = this->get_original_contig_id(Original_Contigs, num_original_contigs, original_contig_name);
        if (original_contig_id < 0)
            throw std::runtime_error(fmt::format("Error: original contig name {} not found in Original_Contigs. file: {}, line: {}\n", original_contig_name, __FILE__, __LINE__));
        int scaff_id = get_scaff_id(scaff_name); // start from 0
        
        if (this->original_contigs.find(original_contig_name) == this->original_contigs.end())
            this->original_contigs[original_contig_name] = Original_Contig_agp(end_local, 1);
        else {
            this->original_contigs[original_contig_name].num_frags++;
            this->original_contigs[original_contig_name].len = std::max(this->original_contigs[original_contig_name].len, end_local);
        }
        this->original_contigs[original_contig_name].starts.push_back(start_local);
        this->frags.emplace_back(original_contig_id, scaff_id, start_local, std::abs(end_local- start_local) + 1, is_reverse, is_painted, meta_tags);

        if (this->scaffs.size() <= frags.back().scaff_id)
            this->scaffs.emplace_back(is_painted);

        this->scaffs[this->frags.back().scaff_id].len += this->frags.back().len;
        this->total_bp += this->frags.back().len;
    }

    /* 
        restore the assembly from the 
            - UN-CORRECTED .agp file
    */
    AssemblyAGP(
        const std::string& agp_file,
        const original_contig* Original_Contigs,
        const int& num_original_contigs,
        meta_data* Meta_Data // used to restore the meta data tag
    )
    {
        std::ifstream file(agp_file);
        if (!file.is_open())
        {
            fmt::print(stderr, "Failed to open AGP file: {}\n", agp_file);
            return;
        }

        sample_name = agp_file.substr(agp_file.find_last_of("/\\") + 1);
        sample_name = sample_name.substr(0, sample_name.find('.'));

        std::string line;
        while (std::getline(file, line))
        {   
            std::smatch match;
            if (line.empty() ) continue; // Skip empty lines and comments
            else if (std::regex_search(line, match, std::regex("^##agp-version\\s*(\\d+.\\d+)\\s*$"))) // version
            {
                this->agp_version = match[1].str();
            } 
            else if (std::regex_search(line, match, std::regex("^# DESCRIPTION:\\s*(.*)$"))) // descirption
            {
                this->description = match[1].str();
            } else if (std::regex_search(line, match, std::regex("^# HiC MAP RESOLUTION:\\s*(\\d+.\\d*)\\s+bp/texel\\s*$")))
            {
                this->bp_per_pixel = std::stod(match[1].str());            
            } else if (std::regex_search(line, match, std::regex("^([\\w_]+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\w+)\\s+([\\w_]+)\\s+(\\d+)\\s+(\\d+)\\s+([+-])\\s*([Painted]*)\\s*(.*)$"))) // genome line
            {
                std::string scaff_name = match[1].str();
                int scaff_start = std::stoi(match[2].str());
                int scaff_end = std::stoi(match[3].str());
                int scaff_frag_id = std::stoi(match[4].str());
                bool is_gap = match[5].str() == "U";
                std::string original_contig_name = match[6].str();
                int start_local = std::stoi(match[7].str());
                int end_local = std::stoi(match[8].str());
                bool is_reverse = match[9].str() == "-";
                bool is_painted = match[10].str() == "Painted";
                uint64_t meta_tags = parse_tags(match[11].str(), Meta_Data);

                this->add_frag(
                    Original_Contigs, num_original_contigs,
                    scaff_name, original_contig_name,
                    start_local, end_local, 
                    is_reverse, is_painted, meta_tags
                );
            } else if (std::regex_search(line, match, std::regex("^([\\w_]+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\w+)\\s+(\\d+)\\s+([\\w_]+)\\s+(\\w+)\\s+([\\w_]+)(.+)$"))) // gap line
            {
                std::string scaff_name = match[1].str();
                int scaff_start = std::stoi(match[2].str());
                int scaff_end = std::stoi(match[3].str());
                int scaff_frag_id = std::stoi(match[4].str());
                bool is_gap = match[5].str() == "U";
                int gap_length = std::stoi(match[6].str());
            }
        }
        file.close();
        this->sort_frags_local_index(Original_Contigs);
    }

    uint64_t parse_tags(const std::string tags_str, meta_data* Meta_Data)
    {   
        if (tags_str.empty()) return 0;
        uint64_t tags_u64 = 0;
        std::istringstream iss(tags_str);
        std::string tag;
        
        // 使用流操作符按空格分割字符串
        while (iss >> tag) 
        {
            for (int i = 0 ; i<64 ; i ++ )
            {   
                if (std::string((char*)Meta_Data->tags[i]).empty()) 
                {   
                    memcpy(Meta_Data->tags[i], tag.data(), tag.size() + 1);
                    tags_u64 |= (1ULL << i);
                    break;
                }
                else if (tag == std::string((char*)(Meta_Data->tags[i])))
                {
                    tags_u64 |= (1ULL << i);
                    break;
                }
            }
        }
        return tags_u64;
    }

    /* validate the number of base pairs */
    int cal_total_bp()
    {
        int total = 0;
        for (const auto& frag : frags)
        {
            total += frag.len;
        }

        int total_bp_scaff = 0;
        for (const auto& scaff : scaffs) 
            total_bp_scaff += scaff.len; 
        if (total != total_bp_scaff)
        {
            fmt::print(stderr, "Warning: total bp in frags ({}) != total bp in scaffs ({})\n", total, total_bp_scaff);
            assert(total == total_bp_scaff);
            return -1;
        }
        return total;
    }

    /*
        restore the map_state from the assembly
            - UN-CORRECTED .agp file
        return: 0 if success, -1 if failed
    */
    int restore_map_state_from_agp(
        const int& num_pix_1d,
        map_state* Map_State,
        const original_contig* Original_Contigs,
        const int& num_original_contigs
    )
    {   
        int pixel_index;
        double start_pixel_global = 0, len_pixel = 0, local_start_pixel;
        for ( int i  = 0 ; i < this->frags.size() ;  i++ )
        {   
            const Frag& frag = frags[i];
            local_start_pixel = (double) frag.start / this->bp_per_pixel;
            len_pixel = (double) frag.len / this->bp_per_pixel;
            for (int j = 0; j < std::round(len_pixel); j ++ )
            {   
                pixel_index = std::round(start_pixel_global) + j;
                if ( pixel_index >= num_pix_1d )
                {
                    fmt::print(stderr, "Warning: pixel index ({:.2f}) out of range [0, {}).\n", pixel_index, num_pix_1d);
                    assert(0);
                    return -1;
                }
                Map_State->contigIds[pixel_index] = i;
                Map_State->originalContigIds[pixel_index] = frag.orig_contig_id;
                Map_State->contigRelCoords[pixel_index] = frag.is_reverse ? 
                    std::round(local_start_pixel + len_pixel) - j - 1: 
                    std::round(local_start_pixel) + j;
                Map_State->scaffIds[pixel_index] = frag.scaff_id;
                pixel_index++;
            }
            start_pixel_global += len_pixel;
        }
        #ifdef DEBUG
            int n = 2000;
            fmt::print("[Load AGP::restore_map_state_from_agp]: Relative coordinates of the first {} pixels:\n", n);
            for (int i = 0; i < n ; i++)
            {
                fmt::print("{}:{} ", Map_State->originalContigIds[i], Map_State->contigRelCoords[i]);
                if (i % 40 == 39) fmt::print("\n");
            }
            fmt::print("\n");

        #endif // DEBUG


        return 0;
    }

    int get_original_contig_id(
        const original_contig* Original_Contigs, const int& num_original_contigs, const std::string& name)
    {
        for (int i = 0; i < num_original_contigs; i++)
        {
            if (strcmp((char*)(Original_Contigs+i)->name, name.c_str()) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    std::string __str__() const
    {
        std::string 
        str =  fmt::format("Sample name:   {}\n", sample_name);
        str += fmt::format("AGP Version:   {}\n", agp_version);
        str += fmt::format("Description:   {}\n", description);
        str += fmt::format("Date:          {}\n", date.to_string());
        str += fmt::format("BP per pixel:  {}\n", bp_per_pixel);
        str += fmt::format("Total BP:      {}\n", total_bp);
        return str;
    }

    void sort_frags_local_index(const original_contig* Original_Contigs)
    {   
        for (auto& it:this->original_contigs) std::sort(it.second.starts.begin(), it.second.starts.end());
        int tmp_index = 0 ;
        for (int i = 0 ; i < frags.size(); i++)
        {
            std::string original_contig_name = std::string((char*)Original_Contigs[frags[i].orig_contig_id].name);
            while (tmp_index < this->original_contigs[original_contig_name].starts.size() && 
                this->frags[i].start != this->original_contigs[original_contig_name].starts[tmp_index]) tmp_index++;
            if (tmp_index >= this->original_contigs[original_contig_name].starts.size())
            {
                throw std::runtime_error(fmt::format("Error: original contig name {} not found in Original_Contigs. file: {}, line: {}\n", original_contig_name, __FILE__, __LINE__));
            }
            this->frags[i].local_index = tmp_index;
            tmp_index = 0;
        }
    }
};

#endif // parse_agp_h


