
#ifndef GENOMEDATA_H
#define GENOMEDATA_H

#include "utilsPretextView.h"
#include "showWindowData.h"



struct
file_atlas_entry
{
    u32 base;  // start
    u32 nBytes;
};


struct contact_matrix
{
    GLuint textures;       // 存储一个或多个OpenGL纹理对象的句柄
    GLuint pixelStartLookupBuffer;
    GLuint pixelRearrangmentLookupBuffer; // save the pixel rearrangement lookup buffer
    GLuint pixelStartLookupBufferTex;
    GLuint pixelRearrangmentLookupBufferTex;
    GLint pad;
    GLuint *vaos;          // 指向顶点数组对象数组
    GLuint *vbos;          // 指向顶点缓冲区对象数组
    GLuint shaderProgram;  // 着色器编号
    GLint matLocation;
};


// one original contig
// which may be split into multiple fragments
struct original_contig    
{
    u32 name[16];         // contig name
    u32 *contigMapPixels; // global center coordinate of the fragments
    u32 nContigs;         // num of contigs
    u32 pad;
};


struct contig
{
    u64 *metaDataFlags;
    u32 length;
    u32 originalContigId; // original contig id
    u32 startCoord;       // local coordinate on the original contig
    u32 scaffId;
    u32 pad;
};


struct contigs
{
    u08 *contigInvertFlags; // invert flag [num_pixels_1d / 8], 1 bit for one pixel
    contig *contigs;
    u32 numberOfContigs;
    u32 pad;
};


struct meta_data
{
    u32 tags[64][16];
};


struct map_state
{
    u32 *contigIds;         // [num_pixels_1d]
    u32 *originalContigIds; // orignal contig id [num_pixels_1d]
    u32 *contigRelCoords;   // local coordinate on the original contig [num_pixels_1d]
    u32 *scaffIds;
    u64 *metaDataFlags;
};



// extension structures
struct
graph
{
    u32 name[16];
    u32 *data;    // save the data of the graph
    GLuint vbo;
    GLuint vao;
    editable_plot_shader *shader;
    f32 scale;
    f32 base;
    f32 lineSize;
    u16 on;
    u16 nameOn;
    nk_colorf colour;
};


enum
extension_type
{
    extension_graph,
};


struct
extension_node
{
    extension_type type;
    u32 pad;
    void *extension;
    extension_node *next;
};


struct
extension_sentinel
{
    extension_node *head;  // point the first node
    extension_node *tail;  // point the last node
};




#endif // GENOMEDATA_H