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
    mode_meta_edit = 4,
    mode_extension = 5
};


struct ShowAutoCurationButton
{

    // Variables to store the current values
    u32 smallest_frag_size_in_pixel = 2;
    f32 link_score_threshold = 0.4f;

    // Variables for the editing UI state
    bool show_yahs_erase_confirm_popup = false;
    bool show_yahs_redo_confirm_popup = false;
    //sorting mode
    u32 sort_mode = 0; // 0: union find, 1: fuse union find, 2 deep fuse
    std::vector<std::string> sort_mode_names = {"Union Find", "Fuse Union Find", "Deep Fuse"};
    u08 frag_size_buf[16];
    u08 score_threshold_buf[16];
    ShowAutoCurationButton()
    {
        set_buf();
    }

    void set_buf()
    {
        memset(frag_size_buf, 0, 16);
        memset(score_threshold_buf, 0, 16);
        snprintf((char*)frag_size_buf, sizeof(frag_size_buf), "%u", smallest_frag_size_in_pixel);
        memset(score_threshold_buf, 0, sizeof(score_threshold_buf));
        snprintf((char*)score_threshold_buf, sizeof(score_threshold_buf), "%.3f", link_score_threshold);
    }

    std::string get_sort_mode_name() const
    {
        return sort_mode_names[sort_mode];
    }
};


#endif // SHOW_WINDOW_DATA_H