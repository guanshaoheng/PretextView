#ifndef FRAGS_ORDER_H
#define FRAGS_ORDER_H

#include "utilsPretextView.h"


struct FragsOrder
{
private:
    u32 num_frags;                  // number of all the fragments
    u32 num_chromosomes;
    u32* num_frags_in_chromosomes = nullptr;  // [num_chromosomes]
    s32** order = nullptr;                    // [num_chromosomes, num_frags_in_chromosomes], + - represents the direction of the fragment, start from 1
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




#endif // FRAGS_ORDER_H
