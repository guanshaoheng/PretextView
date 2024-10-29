/*
Copyright (c) 2021 Ed Harry, Wellcome Sanger Institute

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

#ifndef HEADER_H  // 防止重定义错误
#include "Header.h"
#endif


#define Number_Of_Texture_Buffers_Per_Queue 8
#define Number_Of_Texture_Buffer_Queues 8

// 定义缓冲区结构体
struct texture_buffer 
{
    u08 *texture;                            // 存储纹理数据的指针
    u08 *compressionBuffer;                  // 存储压缩数据的指针
    libdeflate_decompressor *decompressor;   // 解压器指针
    FILE *file;                              // 文件指针，用于读取数据
    u32 homeIndex;                           // 纹理缓冲区索引
    u16 x;                                   // 缓冲区宽度
    u16 y;                                   // 缓冲区高度
    texture_buffer *prev;                    // 前一个缓冲区的指针，用于构建双向链表
};

// 定义单一纹理缓冲区队列
struct single_texture_buffer_queue   
{
    u32 queueLength;          // 队列中缓冲区数目
    u32 pad;                  // 填充字节，用于对齐
    mutex rwMutex;            // 读写互斥锁，防止队列读取中发生错乱  typedef pthread_mutex_t mutex; 采用多线库中的pthread_mutex_t定义而来
    texture_buffer *front;    // 头节点指针
    texture_buffer *rear;     // 尾节点指针
};

// 定义缓冲区队列结构体
struct texture_buffer_queue
{
    single_texture_buffer_queue **queues;  // 多个单一队列缓冲区的指针数组, queues是一个地址，存储的 *queues 是一个地址，这个地址存储一个缓冲区队列**queues，
    threadSig index;                       // 队列的索引 typedef volatile u32 threadSig;
    u32 pad;                               // 填充字节，用于对齐
};

// 定义函数对单个buffer队列进行初始化
global_function
void
InitialiseSingleTextureBufferQueue(single_texture_buffer_queue *queue)
{
    InitialiseMutex(queue->rwMutex);  // 初始化互斥锁 #define InitialiseMutex(x) pthread_mutex_init(&(x), NULL)，将单个队列的互斥锁设置为可用
    queue->queueLength = 0;           // 长度为0表明队列中为空
}

global_function
void
AddSingleTextureBufferToQueue(single_texture_buffer_queue *queue, texture_buffer *buffer)
{   // 将最后buffer添加为queue的rear最后一个 
    LockMutex(queue->rwMutex); // 将当前queue上锁
    buffer->prev = 0;

    switch (queue->queueLength)
    {
        case 0: // 设置为第一个buffer
            queue->front = buffer;  // 此处不定义front的buffer->prev，只是在定一下一个的时候定义
            queue->rear = buffer;
            break;

        default: //紧跟着设置buffer
            queue->rear->prev = buffer; // 设置原来的尾部节点的前一个为buffer
            // ?? 为什么不是 将前一个给到buffer的prev然后将buffer给到queue->rear buffer->prev = queue->rear;
            queue->rear = buffer;       // 将buffer设置为尾节点
    }

    ++queue->queueLength; // 更新buffer长度
    UnlockMutex(queue->rwMutex); // 操作完解锁当前queue
}

#define Compression_Header_Size 128

global_function
void
InitialiseTextureBufferQueue(memory_arena *arena, texture_buffer_queue *queue, u32 nBytesForTextureBuffer, const char *fileName)
{   // 为所有的 single queue， 及其中的texture初始化内存
    queue->queues = PushArrayP(arena, single_texture_buffer_queue *, Number_Of_Texture_Buffer_Queues);  // allocate pointers for 一个queue
    queue->index = 0;      // 设置线程索引为0，后面可能会修改，因为threadsig 为 volatile u32
    u32 nAdded = 0;        // 成功添加decompressor的个数
    u32 nFileHandles = 0;  // 成功添加file指针的个数

    ForLoop(Number_Of_Texture_Buffer_Queues) // 一共有8个queue
    {   // 类似二维数组
        queue->queues[index] = PushStructP(arena, single_texture_buffer_queue); // allocate spaces for each queues
        InitialiseSingleTextureBufferQueue(queue->queues[index]);  // **queue指一个队列变量，*(queue+index)或*queue[index] 表示一个队列变量的指针

        ForLoop2(Number_Of_Texture_Buffers_Per_Queue) // 每个queue会有8个buffer
        {
            texture_buffer *buffer = PushStructP(arena, texture_buffer); // allocate space for texture buffer 
            buffer->texture = PushArrayP(arena, u08, nBytesForTextureBuffer); // space for texture
            buffer->compressionBuffer = PushArrayP(arena, u08, nBytesForTextureBuffer + Compression_Header_Size); // space for buffer and the compression header
            buffer->decompressor = libdeflate_alloc_decompressor(); // decompressor
            buffer->file = fopen(fileName, "rb");    // file pointer
            buffer->homeIndex = index;               // texture的index是第几个队列的地址，每个队列中有8个texture
            if (buffer->decompressor)                // 确保buffer初始化成功，成功分配解压器
            {
                ++nAdded;
                if (buffer->file)                    // 确保成功分配文件指针
                {
                    ++nFileHandles;
                    AddSingleTextureBufferToQueue(queue->queues[index], buffer); // 将这个用于读取 texture 的buffer添加到queue上
                }
            }
        }
    }

    if (!nAdded)
    {
        fprintf(stderr, "Could not allocate memory for libdeflate decompressors\n");
        exit(1);
    }
    if (!nFileHandles)
    {
        fprintf(stderr, "Could not open input file %s\n", (const char *)fileName); exit(1);
    }
}

global_function
void close_file_in_single_queue(single_texture_buffer_queue* queue){
    LockMutex(queue->rwMutex);
    for (texture_buffer* tmp = queue->front; tmp; tmp = tmp->prev){  // iteration to close all of the file and decompressor
        fclose(tmp->file);
        free(tmp->decompressor);
    }
    UnlockMutex(queue->rwMutex);
    return ;
}

global_function
void
CloseTextureBufferQueueFiles(texture_buffer_queue *queue)
{   // 关闭文件阅读器，释放解压器指针
    ForLoop(Number_Of_Texture_Buffer_Queues)
    {
        close_file_in_single_queue(queue->queues[index]);
    }
    return ;
}


global_function
void
AddTextureBufferToQueue(texture_buffer_queue *queue, texture_buffer *buffer)
{   // 将buffer添加到对应的single queue中 
    single_texture_buffer_queue *singleQueue = queue->queues[buffer->homeIndex]; // 得到buffer对应的queue的指针
    AddSingleTextureBufferToQueue(singleQueue, buffer); // 将buffer添加到这个queue的尾部
}

global_function
texture_buffer *
TakeSingleTextureBufferFromQueue(single_texture_buffer_queue *queue)
{   // 从single texture 中获取一个buffer_texture
    // 取single texture 的头部节点
    LockMutex(queue->rwMutex);  // 给single texture上锁
    texture_buffer *buffer = queue->front;

    switch (queue->queueLength)
    {
        case 0:
            break;

        case 1:
            queue->front = 0;
            queue->rear = 0;
            -- queue->queueLength;
            break;

        default:
            queue->front = queue->front->prev;
            --queue->queueLength;
    }

    UnlockMutex(queue->rwMutex);

    return(buffer);
}

global_function
single_texture_buffer_queue *
GetSingleTextureBufferQueue(texture_buffer_queue *queue)
{   // 从所有的queue中获取一个 queue 
    // __atomic_fetch_add 是一个内建函数，用于原子地执行一个加法操作并返回结果。对指定内存位置的值进行原子加法操作，并返回更新后的值。这个函数可以用来在多线程环境下安全地更新共享变量，确保操作的原子性，即在执行加法操作时，不会被其他线程中断或干扰
    u32 index = __atomic_fetch_add(&queue->index, 1, 0) % Number_Of_Texture_Buffer_Queues; 
    return(queue->queues[index]);
}

global_function
texture_buffer *
TakeTextureBufferFromQueue(texture_buffer_queue *queue)
{   // 从总queue中获取一个buffer，首先获取一个queue，然后从中获取一个buffer
    return(TakeSingleTextureBufferFromQueue(GetSingleTextureBufferQueue(queue)));
}

global_function
texture_buffer *
TakeTextureBufferFromQueue_Wait(texture_buffer_queue *queue)
{   
    texture_buffer *buffer = 0;
    while (!buffer) // 如果buffer为空
    {
        buffer = TakeTextureBufferFromQueue(queue);
    } // 如果队列中的buffer都为空则会一直循环
    return(buffer);
}
