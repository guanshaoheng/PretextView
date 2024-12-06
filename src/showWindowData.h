#ifndef SHOW_WINDOW_DATA_H
#define SHOW_WINDOW_DATA_H

#include "utilsPretextView.h"

struct pointi
{
    s32 x;
    s32 y;
};

struct pointui
{
    u32 x, y;
};

struct pointd
{
    f64 x;
    f64 y;
};

struct
point2f
{
    f32 x;
    f32 y;
};

struct quad
{
    point2f corner[2];
};

struct point3f
{
    f32 x;
    f32 y;
    f32 z;
};

struct tool_tip
{
    pointui pixels;       // point coordinate defined in unsigned integers
    point2f worldCoords;  // coors of the word defined in f32
};


struct edit_pixels
{
    pointui pixels;
    point2f worldCoords;
    pointui selectPixels;
    u08 editing : 1;
    u08 selecting : 1;
    u08 scaffSelecting : 1;
    u08 snap : 1;
};

struct tex_vertex
{
    f32 x, y, pad;
    f32 s, t, u;
};


struct vertex
{
    f32 x, y;
};

enum theme
{
    THEME_BLACK,
    THEME_WHITE,
    THEME_RED,
    THEME_BLUE,
    THEME_DARK,
    
    THEME_COUNT
};

struct flat_shader
{
    GLuint shaderProgram;
    GLint matLocation;
    GLint colorLocation;
    u32 pad;
};

struct ui_shader
{
    GLuint shaderProgram;
    GLint matLocation;
};

struct editable_plot_shader
{
    GLuint shaderProgram;
    GLuint yValuesBuffer;
    GLuint yValuesBufferTex;
    GLint matLocation;
    GLint colorLocation;
    GLint yScaleLocation;
    GLint yTopLocation;
    GLint lineSizeLocation;
};

struct quad_data
{
    GLuint *vaos;
    GLuint *vbos;
    u32 nBuffers;
    u32 pad;
};

struct color_maps
{
    GLuint *maps;
    u32 currMap;
    u32 nMaps;
    GLint cpLocation;
    GLfloat controlPoints[3];
    struct nk_image *mapPreviews;
};

struct nk_glfw_vertex
{
    f32 position[2];
    f32 uv[2];
    nk_byte col[4];
};

struct device
{
    u08 *lastContextMemory;
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
};


struct ui_colour_element_bg
{
    u32 on;
    nk_colorf fg;
    nk_colorf bg;
    f32 size;
};


struct ui_colour_element
{
    u32 on;
    nk_colorf bg;
    f32 size;
};

struct edit_mode_colours
{
    nk_colorf preSelect;
    nk_colorf select;
    nk_colorf invSelect;
    nk_colorf fg;
    nk_colorf bg;
};

struct waypoint_mode_data
{
    nk_colorf base;
    nk_colorf selected;
    nk_colorf text;
    nk_colorf bg;
    f32 size;
};

struct meta_mode_data
{
    nk_colorf text;
    nk_colorf bg;
    f32 size;
};

enum global_mode
{
    mode_normal = 0,
    mode_edit = 1,
    mode_waypoint_edit = 2,
    mode_scaff_edit = 3,
    mode_meta_edit = 4
};


#endif // SHOW_WINDOW_DATA_H