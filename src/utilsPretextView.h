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


#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <numeric>
#include <cassert>
#include <vector>
#include <chrono>
#include <unordered_set>
#include <queue>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#include <glad/glad.h>
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#include <GLFW/glfw3.h>
#pragma clang diagnostic pop



#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>


/*
    original version not commented here
*/
// #ifdef _WIN32
// #include <intrin.h>
// #else  // problem is here
// #include <x86intrin.h>  // please use ```arch -x86_64 zsh```, before run the compiling
// #endif

typedef int8_t s08;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// using u08=uint8_t; // 
typedef uint8_t u08;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t memptr;

#define global_function static
#define global_variable static

#define s08_max INT8_MAX
#define s16_max INT16_MAX
#define s32_max INT32_MAX
#define s64_max INT64_MAX

#define s08_min INT8_MIN
#define s16_min INT16_MIN
#define s32_min INT32_MIN
#define s64_min INT64_MIN

#define u08_max UINT8_MAX
#define u16_max UINT16_MAX
#define u32_max UINT32_MAX
#define u64_max UINT64_MAX

#define f32_max MAXFLOAT

#define my_Min(x, y) (x < y ? x : y)
#define my_Max(x, y) (x > y ? x : y)

#define Abs(x) (x > 0 ? x : -x)

#define u08_n (u08_max + 1)

#define Square(x) (x * x)

#define Pow10(x) (IntPow(10, x))
#define Pow2(N) (1 << N)

#define PI 3.141592653589793238462643383279502884195
#define TwoPI 6.283185307179586476925286766559005768391
#define Sqrt2 1.414213562373095048801688724209698078569
#define SqrtHalf 0.7071067811865475244008443621048490392845

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))
#define ForLoop(n) for (u32 index = 0; index < (n); ++index)
#define ForLoop64(n) for (u64 index = 0; index < (n); ++index)
#define ForLoop2(n) for (u32 index2 = 0; index2 < (n); ++index2)
#define ForLoop3(n) for (u32 index3 = 0; index3 < (n); ++index3)
#define ForLoopN(i, n) for (u32 i = 0; i < (n); ++i)
#define TraverseLinkedList(startNode, type) for (type *(node) = (startNode); node; node = node->next)
#define TraverseLinkedList2(startNode, type) for (type *(node2) = (startNode); node2; node2 = node2->next)
#define TraverseLinkedList3(startNode, type) for (type *(node3) = (startNode); node3; node3 = node3->next)

#define ArgCount argc
#define ArgBuffer argv
#define Main s32 main()
#define MainArgs s32 main(s32 ArgCount, const char *ArgBuffer[])
#define EndMain return(0)


#define GLcall(x) clearGL_Error();\
    x;\
    checkGL_Error(#x);


void clearGL_Error();


bool checkGL_Error(const char *msg);


unsigned int texture_id_cal(unsigned int x, unsigned int y, unsigned int number_of_textures_1d);


std::string readShaderSource(std::string shaderFile);


void PrintShaderInfoLog(GLuint shader);


GLuint CreateShader(
    const char *fragmentShaderSource, 
    const char *vertexShaderSource, 
    const char *geometryShaderSource=nullptr);


#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#define NK_PRIVATE
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_STANDARD_IO
#define NK_ZERO_COMMAND_MEMORY
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_INCLUDE_STANDARD_VARARGS
//#define NK_KEYSTATE_BASED_INPUT
#ifndef DEBUG
#define NK_ASSERT(x)
#endif
#include "nuklear.h"
#pragma clang diagnostic pop


void push_nk_style(
    nk_context *ctx, 
    nk_color normal, 
    nk_color hover,
    nk_color active);


void pop_nk_style(nk_context *ctx, u32 num_color_pushed=3);



#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
// Function to get the path to the Resources directory
std::string getResourcesPath();
#endif // __APPLE__


#ifdef __linux__
#include <unistd.h>
#include <limits.h>
std::string getResourcesPath();
#endif

#ifdef _WIN32
#include <windows.h>
std::string getResourcesPath();
#endif


void my_code_position_handler(const char* file, int line, const char* message=nullptr);

#define MY_CHECK(MESSAGE) my_code_position_handler(__FILE__, __LINE__, MESSAGE)


f64 GetTime(); // return current time in seconds


/* 计算数组的 95% 分位数 */
template <typename T>
T percentile_cal(T* data, u32 size, f32 percentile=0.95)
{   
    u32 heap_size = (u32)((f32)size * (1-percentile));
    std::priority_queue<T, std::vector<T>, std::greater<T>> min_heap;
    for (auto i = 0; i < size; i++)
    {
        if (min_heap.size() < heap_size)
        {
            min_heap.push(data[i]);
        }
        else if (data[i] > min_heap.top())
        {
            min_heap.pop();
            min_heap.push(data[i]);
        }
    }
    if (min_heap.size() > 0) 
        return min_heap.top();
    else return 0;
}

#endif // UTILS_H