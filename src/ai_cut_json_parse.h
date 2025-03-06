

#ifndef auto_cut_h
#define auto_cut_h

#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>


#include "utilsPretextView.h"


using json = nlohmann::json;



struct Problem
{   
    int id;
    std::string image_id;
    std::string category;
    std::vector<f32> bbox;       // [x1, y1, x2, y2]
    f32 score;
    f32 resolution;
    std::vector<u64> bp_loci;     // [start1, end1, start2, end2]
    std::vector<u32> hic_loci_pixel; 
    std::string infer_image;
};


void from_json(const json& j, Problem& t) 
{
    j.at("id").get_to(t.id);
    j.at("image_id").get_to(t.image_id);
    j.at("category").get_to(t.category);
    j.at("bbox").get_to(t.bbox);
    j.at("score").get_to(t.score);
    j.at("resolution").get_to(t.resolution);
    j.at("hic_loci").get_to(t.bp_loci);
    j.at("infer_image").get_to(t.infer_image);
    t.hic_loci_pixel = std::vector<u32>(4, 0);
}



class HIC_Problems
{   
public:
    f32 bp_per_pixel;
    std::vector<std::string> types={"translocation", "inversion", "debris"};
    std::map<std::string, std::vector<Problem>> problems;

    HIC_Problems(std::string file_path, f32 bp_per_pixel_)
    : bp_per_pixel(bp_per_pixel_)
    {
        std::ifstream file(file_path, std::ios::in);
        if (!file.is_open())
        {   
            throw std::runtime_error("Cannot open the file: " + file_path);
        }
        
        // 解析 JSON
        json data = json::parse(file);

        for (const auto& i : this->types)
        {
            if (data.contains(i) && data[i].is_array()) 
            {
                for (const auto& item : data[i]) 
                {   
                    auto tmp = item.get<Problem>();
                    if (tmp.bp_loci.size() != 4)
                    {
                        throw std::runtime_error("The hic_loci should contain 4 elements, tmp_id:" + tmp.image_id);
                    }
                    for (int j = 0; j < 4; j++)
                    {
                        tmp.hic_loci_pixel[j] = (u32) ((f32)tmp.bp_loci[j] / bp_per_pixel);
                    }
                    this->problems[i].push_back(tmp);
                }
            } else 
            {   
                #ifdef DEBUG
                    std::cerr << "Warning: No valid " << i << " data found" << std::endl;
                #endif // DEBUG
            }
        }
    }

    std::vector<std::vector<u32>> get_problem_loci()
    {
        std::vector<std::vector<u32>> output;

        for (const auto& type : this->types)
        {
            for (auto item : this->problems[type])
            {   
                output.push_back({
                    item.hic_loci_pixel[0],
                    item.hic_loci_pixel[1]
                });
                std::cout << type << " loci: " << item.hic_loci_pixel[0] << " " << item.hic_loci_pixel[1] << std::endl;
            }
        }
        return output;
    } 


};



#endif // auto_cut_h