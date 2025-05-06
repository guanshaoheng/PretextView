#ifndef SELF_MATRIX_H
#define SELF_MATRIX_H


template<typename T>
class Matrix2D
{
private:
    T* data = nullptr;
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
    T* data = nullptr;
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

        if (data)
        {   
            delete[] data;
            data = nullptr;
        }
    }

    void re_allocate_mem(u32 row_num_, u32 col_num_, u32 layer_)
    {   
        cleanup();
        row_num = row_num_;
        col_num = col_num_;
        layer_num = layer_;
        shape[0] = row_num_;
        shape[1] = col_num_;
        shape[2] = layer_;
        length = row_num * col_num * layer_num;

        data = new T[length];

        memset(data, 0, length * sizeof(T));
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

    void output_to_file(FILE* fp) const
    {
        fmt::print(fp, "# Matrix shape: {} {} {}\n", row_num, col_num, layer_num);
        for (u32 l = 0; l < layer_num; l++)
        {   
            fmt::print(fp, "# layer: {}\n", l);
            for (u32 i = 0; i < row_num; i++)
            {
                for (u32 j = 0; j < col_num; j++)
                {
                    fmt::print(fp, "{:.4f} ", data[i * col_num * layer_num + j * layer_num + l]);
                }
                fmt::print(fp, "\n");
            }
        }
    }

    const T* get_data_ptr() const
    {
        return data;
    }
};


#endif // SELF_MATRIX_H