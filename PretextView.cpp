/*
Copyright (c) 2021 Ed Harry, Wellcome Sanger Institute
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


#define my_String_(x) #x
#define my_String(x) my_String_(x)

#define PretextView_Version "PretextViewAI Version " my_String(PV) // PV is defined in the CMakeLists.txt
#define PretextView_Title "PretextViewAI " my_String(PV) " - Wellcome Sanger Institute"    


#include <frag_sort.h>  // place this before add Header.h to avoid macro conflict
#include <Header.h>

#ifdef DEBUG
    #include <errno.h>
#endif // DEBUG


#include "utilsPretextView.h"
#include "auto_curation_state.h"

#include "TextureLoadQueue.cpp"  // 
#include "ColorMapData.cpp"      // add color maps 

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wshorten-64-to-32"
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "gl3corefontstash.h"
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconditional-uninitialized"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wimplicit-int-conversion"

#define STBI_ONLY_PNG
#ifndef DEBUG
#define STBI_ASSERT(x)
#endif // debug
#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
#endif // STB_IMAGE_IMPLEMENTATION

#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
    #define STB_IMAGE_WRITE_IMPLEMENTATION
    #include "stb_image_write.h"
#endif // STB_IMAGE_WRITE_IMPLEMENTATION

#pragma clang diagnostic pop


#include "Resources.cpp" // defines icons, fonts

#include "genomeData.h"
#include "showWindowData.h"
#include "hic_figure.h"
#include "ai_cut_json_parse.h"
#include "frag_cut_calculation.h"
#include "grey_out_settings.h"
#include "user_profile_settings.h"

#include "shaderSource.h"
/*
// std::string shader_source_dir = getResourcesPath() + "/src/shaderSource/";
std::string shader_source_dir = SHADER_DIR;
// Contact_Matrix->shaderProgram
global_variable
std::string
VertexSource_Texture = readShaderSource(shader_source_dir + "contactMatrixVertex.shader");

// Contact_Matrix->shaderProgram
global_variable
std::string
FragmentSource_Texture = readShaderSource( shader_source_dir + "contactMatrixFragment.shader");

global_variable
std::string
VertexSource_Flat = readShaderSource( shader_source_dir + "flatVertex.shader");

global_variable
std::string
FragmentSource_Flat = readShaderSource( shader_source_dir + "flatFragment.shader");

global_variable
std::string
VertexSource_EditablePlot = readShaderSource( shader_source_dir + "editVertex.shader");

global_variable
std::string
FragmentSource_EditablePlot = readShaderSource( shader_source_dir + "editFragment.shader");

// https://blog.tammearu.eu/posts/gllines/
global_variable
std::string
GeometrySource_EditablePlot = readShaderSource( shader_source_dir + "editGeometry.shader");

global_variable
std::string
VertexSource_UI = readShaderSource( shader_source_dir + "uiVertex.shader");

global_variable
std::string
FragmentSource_UI = readShaderSource( shader_source_dir + "uiFragment.shader");
*/



#define UI_SHADER_LOC_POSITION 0
#define UI_SHADER_LOC_TEXCOORD 1
#define UI_SHADER_LOC_COLOR 2


global_variable
memory_arena
Working_Set;

global_variable
thread_pool *
Thread_Pool;

global_variable
u32
Redisplay = 0;

global_variable
s32
Window_Width, Window_Height, FrameBuffer_Width, FrameBuffer_Height;

global_variable
struct nk_vec2
Screen_Scale;

global_variable
theme
Current_Theme;

global_variable
u08 *
Theme_Name[THEME_COUNT];

global_function
void
SetTheme(struct nk_context *ctx, enum theme theme);



global_variable
    f32
        bgcolor[][4] =
            {
                {0.2f, 0.6f, 0.4f, 1.0f},       // original
                {0.922f, 0.635f, 0.369f, 1.0f}, // jasper (orange)
                {0.62f, 0.482f, 0.71f, 1.0f},   // heather (lavender)
                {0.29f, 0.545f, 0.659f, 1.0f},  // Carolina (blue)
                {1.0f, 1.0f, 1.0f, 1.0f},       // white
                {0.765f, 0.765f, 0.765f, 1.0f}, // Grey
                {0.0f, 0.0f, 0.0f, 1.0f}        // Black
};

global_variable const char *
    bg_color[] =
        {
            "Classic",
            "Jasper",
            "Heather",
            "Carolina",
            "White",
            "Grey",
            "Black"};

global_variable
    u08
        active_bgcolor = 0;

struct
    metaoutline
{
    u08 on;
    u08 color;
};

struct metaoutline global_meta_outline = {1, 0};

global_variable metaoutline *
    meta_outline = &global_meta_outline;

global_variable
    u08
        default_metadata_colorProfile = 0;

global_variable
    u08
        meta_data_curcolorProfile = default_metadata_colorProfile;

global_variable
    f32
        meta_dataColors[][3] =
            {
                {1.666f, 2.666f, 3.666f}, // original
                {2.666f, 1.666f, 3.666f},
                {3.666f, 2.666f, 1.666f},
                {5.666f, 3.666f, 2.666f}};

global_variable const char *
    metaColors[] =
        {
            "Color set 1",
            "Color set 2",
            "Color set 3",
            "Color set 4"};

global_variable const char *
    outlineColors[] =
        {
            "No-Outline",
            "Black-Outline",
            "White-Outline"};

global_variable
    u08
        activeGraphColour = 0;

global_variable
    nk_colorf
        graphColors[] =
            {
                {0.1f, 0.8f, 0.7f, 1.0f},       // Default
                {1.0f, 0.341f, 0.2f, 1.0f},     // Vermillion
                {0.137f, 0.451f, 0.882f, 1.0f}, // essence (vibrant blue)
                {0.706f, 0.0f, 1.0f, 1.0f}      // Drystorm (purple)
};

global_variable const char *
    colour_graph[] =
        {
            "Default",
            "Vermillion",
            "Blue",
            "Purple"};

global_variable
    nk_colorf
        DefaultGraphColour = graphColors[activeGraphColour];


global_variable
nk_context *
NK_Context;

global_function
void
ChangeSize(s32 width, s32 height)
{
    glViewport(0, 0, width, height);
}

global_function
void
UpdateScreenScale()
{
    Screen_Scale.x = (f32)FrameBuffer_Width / (f32)Window_Width;
    Screen_Scale.y = (f32)FrameBuffer_Height / (f32)Window_Height;
    SetTheme(NK_Context, Current_Theme);
}

global_function
void
GLFWChangeFrameBufferSize(GLFWwindow *win, s32 width, s32 height)
{
    (void)win;
    ChangeSize(width, height);
    Redisplay = 1;
    FrameBuffer_Height = height;
    FrameBuffer_Width = width;
    UpdateScreenScale();
}

global_function
void
GLFWChangeWindowSize(GLFWwindow *win, s32 width, s32 height)
{
    (void)win;
    Window_Width = width;
    Window_Height = height;
    UpdateScreenScale();
}


global_variable
device *
NK_Device;

global_variable
nk_font_atlas *
NK_Atlas;

global_variable
nk_font *
NK_Font;

global_variable
nk_font *
NK_Font_Browser;

global_variable
color_maps *
Color_Maps;

#ifdef Internal
global_variable
quad_data *
Texture_Tile_Grid;

global_variable
quad_data *
QuadTree_Data;
#endif


global_variable
Extension_Graph_Data * pixel_density_extension = nullptr;

global_variable
TexturesArray4AI* textures_array_ptr = nullptr;

global_variable
FragCutCal* frag_cut_cal_ptr = nullptr;


struct
    CustomColorMapOrder
{
    u32 nMaps;
    u32 order[Number_of_Color_Maps];
};


global_variable
    CustomColorMapOrder
        userColourMapOrder;


global_variable
s32
useCustomOrder = 0;


global_variable
quad_data *
Grid_Data;

global_variable
quad_data *
Contig_ColourBar_Data;

global_variable
quad_data *
Scaff_Bar_Data;

global_variable
quad_data *
Edit_Mode_Data;

global_variable
quad_data *
Label_Box_Data;

global_variable
quad_data *
Scale_Bar_Data;

global_variable
quad_data *
Tool_Tip_Data;

global_variable
quad_data *
Waypoint_Data;

global_variable
ui_shader *
UI_Shader;

global_variable
flat_shader *
Flat_Shader;

global_variable
contact_matrix *
Contact_Matrix;

global_variable
threadSig
Texture_Ptr = 0; // used to define which texture is currently being processed, the data is saved in Current_Loaded_Texture

global_variable
volatile texture_buffer *
Current_Loaded_Texture = 0; // define the global texture buffer 

global_variable
u32
Texture_Resolution;

global_variable
u32
Number_of_MipMaps;

global_variable
u32
Number_of_Textures_1D;

global_variable
u32
Number_of_Pixels_1D;

global_variable
u32
Bytes_Per_Texture;

global_variable
texture_buffer_queue *
Texture_Buffer_Queue;


global_variable
pointd
Mouse_Move;


global_variable
tool_tip Tool_Tip_Move; 


global_variable
edit_pixels
Edit_Pixels;


global_variable
point3f
Camera_Position;


global_variable
FONScontext *
FontStash_Context;


#define Grey_Background {0.569f, 0.549f, 0.451f, 1.0f}
#define Yellow_Text_Float {0.941176471f, 0.725490196f, 0.058823529f, 1.0f}
#define Red_Text_Float {0.941176471f, 0.039215686f, 0.019607843f, 1.0f}

#define Green_Float {0.3f, 0.6f, 0.0f, 0.2f}
#define Red_Float {0.6f, 0.3f, 0.0f, 0.2f}
#define Blue_Float {0.0f, 0.3f, 0.6f, 0.2f}

#define Red_Full {1.0f, 0.0f, 0.0f, 1.0f}
#define Blue_Full {0.0f, 0.0f, 1.0f, 1.0f}

global_variable
ui_colour_element_bg *
Contig_Name_Labels;

global_variable
ui_colour_element_bg *
Scale_Bars;

global_variable
ui_colour_element_bg *
Tool_Tip;

global_variable
u32
Waypoints_Always_Visible = 1;

global_variable
u08
Scaffs_Always_Visible = 1;

global_variable
u08
MetaData_Always_Visible = 1;


global_variable
ui_colour_element *
Grid;

global_variable
ui_colour_element *
Contig_Ids;

#ifdef Internal
global_variable
ui_colour_element *
Tiles;

global_variable
ui_colour_element *
QuadTrees;
#endif


global_variable
edit_mode_colours *
Edit_Mode_Colours;


global_variable
waypoint_mode_data *
Waypoint_Mode_Data;

global_variable
meta_mode_data *
Scaff_Mode_Data;

global_variable
meta_mode_data *
MetaData_Mode_Data;

global_variable
meta_mode_data *
Extension_Mode_Data;


/*
用于显示在input sequences中点击后选中的片段的名字，
显示label持续 5 秒钟
*/
struct selected_sequence_cover_countor
{
    s32 original_contig_index;
    s32 idx_within_original_contig;
    f64 end_time;
    f64 label_last_seconds = 5.0f;
    u08 plotted;
    f32 contig_start; // ratio
    f32 contig_end;   // ratio
    s32 map_loc;      // in pixel
    void clear()
    {
        original_contig_index = -1;
        idx_within_original_contig = -1;
        end_time = -1.;
        plotted = false;
        contig_start = -1.0;
        contig_end = -1.0;
        map_loc = -1;
    };

    void set(
        s32 original_contig_index_,
        s32 idx_within_original_contig_,
        f64 current_time_,
        f32 contig_start_,
        f32 contig_end_,
        s32 map_loc_
    )
    {   
        this->clear();
        original_contig_index = original_contig_index_; 
        idx_within_original_contig = idx_within_original_contig_; 
        end_time = current_time_ + label_last_seconds;
        plotted = 0;
        contig_start = contig_start_;
        contig_end = contig_end_;
        map_loc = map_loc_;
    }
};


global_variable
selected_sequence_cover_countor
Selected_Sequence_Cover_Countor;



global_variable
u32
UI_On = 0;

global_variable
global_mode
Global_Mode = mode_normal;

#define Edit_Mode (Global_Mode == mode_edit)
#define Normal_Mode (Global_Mode == mode_normal)
#define Waypoint_Edit_Mode (Global_Mode == mode_waypoint_edit)
#define Scaff_Edit_Mode (Global_Mode == mode_scaff_edit)
#define MetaData_Edit_Mode (Global_Mode == mode_meta_edit)
#define Extension_Mode (Global_Mode == mode_extension)
#define Select_Sort_Area_Mode (Global_Mode == mode_selectExclude_sort_area)

global_variable
s32
Font_Normal = FONS_INVALID;

global_variable
s32
Font_Bold = FONS_INVALID;

global_function
void
ClampCamera()
{
    Camera_Position.z = my_Min(my_Max(Camera_Position.z, 1.0f), ((f32)(Pow2((Number_of_MipMaps + 1)))));
    Camera_Position.x = my_Min(my_Max(Camera_Position.x, -0.5f), 0.5f);
    Camera_Position.y = my_Min(my_Max(Camera_Position.y, -0.5f), 0.5f);
}

global_function
void
ZoomCamera(f32 dir)
{
    f32 scrollFactor = 1.1f;
    scrollFactor += dir > 0.0f ? dir : -dir;
    Camera_Position.z *= (dir > 0.0f ? scrollFactor : (1.0f / scrollFactor));
    ClampCamera();
}

global_variable
u32
Loading = 0;


global_variable
u32
auto_sort_state = 0;

global_variable
u32 
auto_cut_state = 0;

global_variable s32 auto_cut_button = 0; 
global_variable s32 auto_sort_button = 0;


global_function
s32
RearrangeMap(u32 pixelFrom, u32 pixelTo, s32 delta, u08 snap = 0, bool update_contigs_flag=true);

global_function
void
InvertMap(u32 pixelFrom, u32 pixelTo, bool update_contigs_flag=true);

global_variable
u32
Global_Edit_Invert_Flag = 0;

global_variable
u08
Scaff_Painting_Flag = 0;

global_variable
u08
Scaff_FF_Flag = 0;

global_variable
u32
Scaff_Painting_Id = 0;


global_variable
original_contig *
Original_Contigs;

global_variable
u32
Number_of_Original_Contigs;




global_variable
contigs *
Contigs;

global_function
u08
IsContigInverted(u32 index)
{
    return(Contigs->contigInvertFlags[index >> 3] & (1 << (index & 7)));  // 所以这个32位的数：后三位表示1所在的位数，其他的表示编号
    // return(Contigs->contigInvertFlags[index / 8] & (1 << (index % 8)));  // check if contig is inverted  
}

global_function
void
setContactMatrixVertexArray(contact_matrix* Contact_Matrix_, bool copy_flag=false, bool regenerate_flag=false);


global_function
void 
prepare_before_copy(f32 *original_color_control_points);


global_function
void 
restore_settings_after_copy(const f32 *original_color_control_points);



global_variable
meta_data *
Meta_Data; // meta data tags for each contig

global_variable
u32
MetaData_Active_Tag = 0;

global_variable
u08
MetaData_Edit_State = 0;

global_variable
u16
sortMetaEdits;


global_variable
const char *
Default_Tags[] = 
{
    "Haplotig",
    "Unloc",
    "X",
    "Y",
    "Z",
    "W",
    "HAP1",
    "HAP2",
    "Target",
    "Contaminant",
    "Singleton",
    "X1",
    "X2",
    "Y1",
    "Y2",
    "Z1",
    "Z2",
    "W1",
    "W2",
    "I",
    "II",
    "III",
    "IV",
    "V",
    "U",
    "B1",
    "B2",
    "B3", 
    "FalseDuplicate",
    "Primary"
};


global_variable
map_state *
Map_State;


#include "copy_texture.h"
// define the struct to store the ai model mask
FragSortTool* frag_sort_method = new FragSortTool();
global_variable auto auto_curation_state = AutoCurationState();


global_function
void
UpdateContigsFromMapState()  //  reading 从map的状态更新contigs
{
    u32 lastScaffID = Map_State->scaffIds[0];       // 第一个scaff的编号
    u32 scaffId = lastScaffID ? 1 : 0;              // 
    u32 lastId = Map_State->originalContigIds[0];   // 第一个像素点对应的id
    u32 lastCoord = Map_State->contigRelCoords[0];  // 第一个像素点的局部坐标
    u32 contigPtr = 0;
    u32 length = 0;
    u32 startCoord = lastCoord;
    u08 inverted = Map_State->contigRelCoords[1] < lastCoord;  // 判断是不是反转的
    Map_State->contigIds[0] = 0;
    
    u32 pixelIdx = 0;
    ForLoop(Number_of_Original_Contigs) (Original_Contigs + index)->nContigs = 0; // 将每一个contig的 片段数目 置为零
    ForLoop(Number_of_Pixels_1D - 1)    // 遍历每一个像素点 更新 Original_Contigs， Contigs 
    // 遍历完之后，contigPtr为214，但是Number_of_Original_Contigs = 218 
    {
        if (contigPtr == Max_Number_of_Contigs) break;  // 确保 contigPtr 不超出最大contig的数值

        ++length; // current fragment length

        pixelIdx = index + 1;   // 像素点编号， 加一因为第一个已经用来初始化了
        u32 id = Map_State->originalContigIds[pixelIdx];  // 像素点的 original contig id, 这里 不使用 % Max_Number_of_Contigs的值是为了区分后面cut之后的片段
        u32 coord = Map_State->contigRelCoords[pixelIdx]; // 像素点的局部坐标
        
        if (
            id != lastId || 
            (inverted && coord != (lastCoord - 1)) || 
            (!inverted && coord != (lastCoord + 1)) 
        ) // 如果不是一个连续片段
        {   
            Original_Contigs[lastId % Max_Number_of_Contigs].contigMapPixels[Original_Contigs[lastId%Max_Number_of_Contigs].nContigs] = pixelIdx - 1 - (length >> 1);   // update Original_Contigs: contigMapPixels
            Original_Contigs[lastId % Max_Number_of_Contigs].nContigs++; // update Original_Contigs: nContigs, contigMapPixels

            contig *last_cont = Contigs->contigs_arr + contigPtr; // 获取上一个contig的指针， 并且给contigPtr + 1
            contigPtr++;
            last_cont->originalContigId = lastId; // 更新这个片段的id
            last_cont->length = length;           // 更新长度
            last_cont->startCoord = startCoord;   // 更新开头为当前片段在该contig上的局部坐标 endCoord = startCoord + length - 1
            last_cont->metaDataFlags = Map_State->metaDataFlags + pixelIdx - 1; // Finished (shaoheng): memory problem: assign the pointer to the cont->metaDataFlags, the original is nullptr, the let this ptr point to the last pixel of the contig

            u32 thisScaffID = Map_State->scaffIds[pixelIdx - 1]; // 上一个像素点对应的 scaffid
            last_cont->scaffId = thisScaffID ? ((thisScaffID == lastScaffID) ? (scaffId) : (++scaffId)) : 0;  // 如果存在scaffid则（判断是不是同一个scaff，如果是则继续用scaffid，否则++scaffid），否则为0  
            lastScaffID = thisScaffID; // 更新


            // 余数表示8数的第几位，如果未反向则对应位为0，若反向则对应位为1
            if (IsContigInverted(contigPtr - 1)) // 判断上一个contig是否反向
            {   // 位操作更新contigflag
                // 每8个片段的正反采用一个u08表示，每一个bit表示一个正反，余数表示这八个中的第几个，如果没有反向则对应位为0
                if (!inverted) Contigs->contigInvertFlags[(contigPtr - 1) >> 3] &= ~(1 << ((contigPtr - 1) & 7));  
            }
            else
            {   // 如果反向则额对应位的bit为1
                if (inverted) Contigs->contigInvertFlags[(contigPtr - 1) >> 3] |= (1 << ((contigPtr - 1) & 7));  // 如果反向
            }

            startCoord = coord; // 当前片段开始的坐标
            length = 0;         // 当前片段长度清零0
            if (pixelIdx < (Number_of_Pixels_1D - 1)) inverted = Map_State->contigRelCoords[pixelIdx + 1] < coord;  // 更新inverted
        }
        // 更新上一个id和局部坐标
        Map_State->contigIds[pixelIdx] = (u32)contigPtr; // 像素点对应的 片段id 修改为当前的统计得到片段id
        lastId = id;       // 更新上一个像素点的id
        lastCoord = coord; // 更新上一个像素点的局部坐标
    }

    if (contigPtr < Max_Number_of_Contigs) //  contigptr 小于 Number_of_Original_Contigs
    // 更新最后一个contig的最后一个片段信息
    {
        (Original_Contigs + lastId % Max_Number_of_Contigs)->contigMapPixels[(Original_Contigs + lastId % Max_Number_of_Contigs)->nContigs++] = pixelIdx - 1 - (length >> 1); 

        ++length;
        contig *cont = Contigs->contigs_arr + contigPtr++;
        cont->originalContigId = lastId;
        cont->length = length;
        cont->startCoord = startCoord;
        cont->metaDataFlags = Map_State->metaDataFlags + pixelIdx - 1;
        
        u32 thisScaffID = Map_State->scaffIds[pixelIdx];
        cont->scaffId = thisScaffID ? ((thisScaffID == lastScaffID) ? (scaffId) : (++scaffId)) : 0;
        
        if (IsContigInverted(contigPtr - 1))
        {
            if (!inverted) Contigs->contigInvertFlags[(contigPtr - 1) >> 3] &= ~(1 << ((contigPtr - 1) & 7));
        }
        else
        {
            if (inverted) Contigs->contigInvertFlags[(contigPtr - 1) >> 3] |= (1 << ((contigPtr - 1) & 7));
        }
    }

    Contigs->numberOfContigs = contigPtr;
}

global_function
void
AddMapEdit(s32 delta, pointui finalPixels, u32 invert);

global_function
void
RebuildContig(u32 pixel)
{
    for (;;)
    {
        u32 contigId = Map_State->contigIds[pixel];
        u32 origContigId = Map_State->get_original_contig_id(pixel);

        u32 top = (u32)pixel;
        while (top && (Map_State->contigIds[top - 1] == contigId)) --top;

        u32 bottom = pixel;
        while ((bottom < (Number_of_Pixels_1D - 1)) && (Map_State->contigIds[bottom + 1] == contigId)) ++bottom;

        if (IsContigInverted(contigId))
        {
            InvertMap(top, bottom);
            AddMapEdit(0, {top, bottom}, 1);
            continue;
        }

        u08 fragmented = 0;
        ForLoop(Number_of_Pixels_1D)
        {
            if ((Map_State->contigIds[index] != contigId) && (Map_State->get_original_contig_id(index) == origContigId))
            {
                fragmented = 1;
                break;
            }
        }

        if (fragmented)
        {
            u32 contigTopCoord = Map_State->contigRelCoords[top];
            if (contigTopCoord)
            {
                u32 otherPixel = 0;
                ForLoop(Number_of_Pixels_1D)
                {
                    if ((Map_State->get_original_contig_id(index) == origContigId) && (Map_State->contigRelCoords[index] == (contigTopCoord - 1)))
                    {
                        otherPixel = index;
                        break;
                    }
                }

                u08 invert = !otherPixel || (Map_State->contigIds[otherPixel - 1] != Map_State->contigIds[otherPixel]);
                u32 otherPixel2 = otherPixel;

                if (invert)
                {
                    while ((otherPixel < (Number_of_Pixels_1D - 1)) && (Map_State->contigIds[otherPixel + 1] == Map_State->contigIds[otherPixel2])) ++otherPixel;
                }
                else
                {
                    while (otherPixel2 && (Map_State->contigIds[otherPixel2 - 1] == Map_State->contigIds[otherPixel])) --otherPixel2;
                }

                s32 delta = (s32)top - (s32)otherPixel;
                if (delta > 0) --delta;
                else delta = (s32)top - (s32)otherPixel2;
                pointui finalPixels = {(u32)((s32)otherPixel2 + delta), (u32)((s32)otherPixel + delta)};
                RearrangeMap(otherPixel2, otherPixel, delta);
                if (invert) InvertMap(finalPixels.x, finalPixels.y);
                AddMapEdit(delta, finalPixels, invert);
            }
            else
            {
                u32 contigBottomCoord = Map_State->contigRelCoords[bottom];

                u32 otherPixel = 0;
                ForLoop(Number_of_Pixels_1D)
                {
                    if ((Map_State->get_original_contig_id(index) == origContigId) && (Map_State->contigRelCoords[index] == (contigBottomCoord + 1)))
                    {
                        otherPixel = index;
                        break;
                    }
                }

                u08 invert = (otherPixel == (Number_of_Pixels_1D - 1)) || (Map_State->contigIds[otherPixel + 1] != Map_State->contigIds[otherPixel]);
                u32 otherPixel2 = otherPixel;

                if (!invert)
                {
                    while ((otherPixel < (Number_of_Pixels_1D - 1)) && (Map_State->contigIds[otherPixel + 1] == Map_State->contigIds[otherPixel2])) ++otherPixel;
                }
                else
                {
                    while (otherPixel2 && (Map_State->contigIds[otherPixel2 - 1] == Map_State->contigIds[otherPixel])) --otherPixel2;
                }

                s32 delta = (s32)bottom - (s32)otherPixel2;
                if (delta < 0) ++delta;
                else delta = (s32)bottom - (s32)otherPixel;
                pointui finalPixels = {(u32)((s32)otherPixel2 + delta), (u32)((s32)otherPixel + delta)};
                RearrangeMap(otherPixel2, otherPixel, delta);
                if (invert) InvertMap(finalPixels.x, finalPixels.y);
                AddMapEdit(delta, finalPixels, invert);
            }

            continue;
        }
        else break;
    }
}

struct
map_edit
{
    u32 finalPix1;
    u32 finalPix2;
    s32 delta;
};

struct
waypoint;

struct
waypoint_quadtree_node
{
    waypoint *wayp;
    waypoint_quadtree_node *next;
    waypoint_quadtree_node *prev;
};

struct
waypoint
{
    point2f coords;
    f32 z;
    u32 index;
    waypoint *prev;
    waypoint *next;
    waypoint_quadtree_node *node;
    // u16 long_waypoint_mode;
};

#define Edits_Stack_Size 32768
#define Waypoints_Stack_Size 128

struct
map_editor
{
    map_edit *edits;
    u32 nEdits;
    u32 editStackPtr;
    u32 nUndone;
    u32 pad;
};

global_variable
map_editor *
Map_Editor;

#define Waypoints_Quadtree_Levels 5

struct
waypoint_quadtree_level
{
#ifdef Internal
    u32 show;
#else
    u32 pad;
#endif
    f32 size;
    point2f lowerBound;
    waypoint_quadtree_level *children[4];
    waypoint_quadtree_node headNode;
};

struct
waypoint_editor
{
    waypoint freeWaypoints;
    waypoint activeWaypoints;
    waypoint_quadtree_level *quadtree;
    waypoint_quadtree_node freeNodes;
    u32 nWaypointsActive;
    u32 pad;
};

global_variable
waypoint_editor *
Waypoint_Editor;

global_variable u16 Long_Waypoints_Mode = 0;  // mode 0: vertical, mode 1: horizontal, mode 2: both

#define Waypoint_Select_Distance 8.0f
global_variable
waypoint *
Selected_Waypoint = 0;

global_function
waypoint_quadtree_level *
PushQuadTree(memory_arena *arena, u32 level = 0, point2f lowerBound = {-0.5f, -0.5f}, f32 size = 1.0f)
{
    waypoint_quadtree_level *quadtreeLevel = 0;

    if (level < Waypoints_Quadtree_Levels)
    {
        quadtreeLevel = PushStructP(arena, waypoint_quadtree_level);
        quadtreeLevel->size = size;
        quadtreeLevel->lowerBound = lowerBound;
        quadtreeLevel->headNode = {};

#ifdef Internal
        quadtreeLevel->show = 0;
#endif

        f32 halfSize = size * 0.5f;
        quadtreeLevel->children[0] = PushQuadTree(arena, level + 1, {lowerBound.x, lowerBound.y}, halfSize);
        quadtreeLevel->children[1] = PushQuadTree(arena, level + 1, {lowerBound.x, lowerBound.y + halfSize}, halfSize);
        quadtreeLevel->children[2] = PushQuadTree(arena, level + 1, {lowerBound.x + halfSize, lowerBound.y}, halfSize);
        quadtreeLevel->children[3] = PushQuadTree(arena, level + 1, {lowerBound.x + halfSize, lowerBound.y + halfSize}, halfSize);
    }

    return(quadtreeLevel);
}

global_function
void
#ifdef Internal
GetWaypointsWithinRectange(point2f lowerBound, point2f size, waypoint ***bufferPtr, u32 reset = 1);
#else
GetWaypointsWithinRectange(point2f lowerBound, point2f size, waypoint ***bufferPtr);
#endif

global_function
void
UpdateWayPoint(waypoint *wayp, point2f coords);

global_function
void
MoveWayPoints(map_edit *edit, u32 undo = 0);

global_function
void
AddMapEdit(s32 delta, pointui finalPixels, u32 invert)
{
    ++Map_Editor->nEdits;
    Map_Editor->nUndone = 0;

    map_edit *edit = Map_Editor->edits + Map_Editor->editStackPtr++;

    if (Map_Editor->editStackPtr == Edits_Stack_Size)
    {
        Map_Editor->editStackPtr = 0;
    }

    u32 pix1 = (u32)(invert ? my_Max(finalPixels.x, finalPixels.y) : my_Min(finalPixels.x, finalPixels.y));
    u32 pix2 = (u32)(invert ? my_Min(finalPixels.x, finalPixels.y) : my_Max(finalPixels.x, finalPixels.y));

    edit->delta = (s32)delta;
    edit->finalPix1 = pix1;
    edit->finalPix2 = pix2;
}

global_function
void
UpdateScaffolds()
{
    ForLoop(Number_of_Pixels_1D) Map_State->scaffIds[index] = (Contigs->contigs_arr + Map_State->contigIds[index])->scaffId;
}

global_function
void
UndoMapEdit()
{
    if (Map_Editor->nEdits && !Edit_Pixels.editing)
    {
        --Map_Editor->nEdits;
        ++Map_Editor->nUndone;

        if (!Map_Editor->editStackPtr)
        {
            Map_Editor->editStackPtr = Edits_Stack_Size + 1;
        }

        map_edit *edit = Map_Editor->edits + (--Map_Editor->editStackPtr);

        if (edit->finalPix1 > edit->finalPix2)
        {
            InvertMap((u32)edit->finalPix1, (u32)edit->finalPix2);
        }

        u32 start = my_Min(edit->finalPix1, edit->finalPix2);
        u32 end = my_Max(edit->finalPix1, edit->finalPix2);

        RearrangeMap(start, end, -edit->delta);

        UpdateScaffolds();
    }
}

global_function
void
RedoMapEdit()
{
    if (Map_Editor->nUndone && !Edit_Pixels.editing)
    {
        ++Map_Editor->nEdits;
        --Map_Editor->nUndone;

        map_edit *edit = Map_Editor->edits + Map_Editor->editStackPtr++;

        if (Map_Editor->editStackPtr == Edits_Stack_Size)
        {
            Map_Editor->editStackPtr = 0;
        }

        u32 start = my_Min(edit->finalPix1, edit->finalPix2);
        u32 end = my_Max(edit->finalPix1, edit->finalPix2);

        RearrangeMap((u32)((s32)start - edit->delta), (u32)((s32)end - edit->delta), edit->delta);
        
        if (edit->finalPix1 > edit->finalPix2)
        {
            InvertMap((u32)edit->finalPix1, (u32)edit->finalPix2);
        }
        
        UpdateScaffolds();
    }
}


global_function
void
MoveWayPoints(map_edit *edit, u32 undo)
{
    pointui tmp;
    s32 delta;
    pointui finalPixels;
    if (undo)
    {
        tmp = {(u32)edit->finalPix1, (u32)edit->finalPix2};
        delta = -(s32)edit->delta;
        finalPixels = {(u32)((s32)tmp.x + delta), (u32)((s32)tmp.y + delta)};
    }
    else
    {
        tmp = {(u32)((s32)edit->finalPix1 - (s32)edit->delta), (u32)((s32)edit->finalPix2 - (s32)edit->delta)};
        delta = (s32)edit->delta;
        finalPixels = {(u32)edit->finalPix1, (u32)edit->finalPix2};
    }

    pointui startPixels = {my_Min(tmp.x, tmp.y), my_Max(tmp.x, tmp.y)};
    u32 lengthMove = startPixels.y - startPixels.x + 1;
    pointui editRange;

    if (delta > 0)
    {
        editRange.x = startPixels.x;
        editRange.y = my_Max(finalPixels.x, finalPixels.y);
    }
    else
    {
        editRange.x = my_Min(finalPixels.x, finalPixels.y);
        editRange.y = startPixels.y;
    }

    f32 ooNPixels = (f32)(1.0 / (f64)Number_of_Pixels_1D);
    point2f editRangeModel = {((f32)editRange.x * ooNPixels) - 0.5f, ((f32)editRange.y * ooNPixels) - 0.5f};
    f32 dRange = editRangeModel.y - editRangeModel.x;

    waypoint **searchBuffer = PushArray(Working_Set, waypoint*, Waypoints_Stack_Size);
    waypoint **bufferEnd = searchBuffer;

    GetWaypointsWithinRectange({editRangeModel.x, -0.5f}, {dRange, 0.999f}, &bufferEnd);
    GetWaypointsWithinRectange({-0.5f, editRangeModel.x}, {0.999f, dRange}, &bufferEnd
#ifdef Internal
, 0
#endif
            );

    u32 nWayp = (u32)((my_Max((size_t)bufferEnd, (size_t)searchBuffer) - my_Min((size_t)bufferEnd, (size_t)searchBuffer)) / sizeof(searchBuffer));
    waypoint **seen = PushArray(Working_Set, waypoint*, nWayp);
    u32 seenIdx = 0;

    for (   waypoint **waypPtr = searchBuffer;
            waypPtr != bufferEnd;
            ++waypPtr )
    {
        waypoint *wayp = *waypPtr;
        u32 newWayP = 1;

        ForLoop(seenIdx)
        {
            if (wayp == seen[index])
            {
                newWayP = 0;
                break;
            }
        }

        if (newWayP)
        {
            point2f normCoords = {my_Min(wayp->coords.x, wayp->coords.y), my_Max(wayp->coords.x, wayp->coords.y)};

            u32 inXRange = normCoords.x >= editRangeModel.x && normCoords.x < editRangeModel.y;
            u32 inYRange = normCoords.y >= editRangeModel.x && normCoords.y < editRangeModel.y;
            u32 inRange = inXRange || inYRange;

            if (inRange)
            {
                u32 upperTri = wayp->coords.x > wayp->coords.y;
                
                pointui pix = {(u32)((0.5f + wayp->coords.x) / ooNPixels), (u32)((0.5f + wayp->coords.y) / ooNPixels)};

                if (pix.x >= startPixels.x && pix.x < startPixels.y)
                {
                    pix.x = (u32)((s32)pix.x + delta);

                    if (edit->finalPix1 > edit->finalPix2)
                    {
                        pix.x = my_Max(finalPixels.x, finalPixels.y) - pix.x + my_Min(finalPixels.x, finalPixels.y);
                    }
                }
                else if (pix.x >= editRange.x && pix.x < editRange.y)
                {
                    pix.x = delta > 0 ? pix.x - lengthMove : pix.x + lengthMove;
                }

                if (pix.y >= startPixels.x && pix.y < startPixels.y)
                {
                    pix.y = (u32)((s32)pix.y + delta);

                    if (edit->finalPix1 > edit->finalPix2)
                    {
                        pix.y = my_Max(finalPixels.x, finalPixels.y) - pix.y + my_Min(finalPixels.x, finalPixels.y);
                    }
                }
                else if (pix.y >= editRange.x && pix.y < editRange.y)
                {
                    pix.y = delta > 0 ? pix.y - lengthMove : pix.y + lengthMove;
                }

                point2f newCoords = {((f32)pix.x * ooNPixels) - 0.5f, ((f32)pix.y * ooNPixels) - 0.5f};

                u32 upperTriNew = newCoords.x > newCoords.y;
                if (upperTriNew != upperTri)
                {
                    newCoords = {newCoords.y, newCoords.x};
                }

                UpdateWayPoint(wayp, newCoords);
            }

            seen[seenIdx++] = wayp;    
        }
    }

    FreeLastPush(Working_Set); // searchBuffer
    FreeLastPush(Working_Set); // seen
}

global_function
void
GetWaypointsWithinRectange(waypoint_quadtree_level *level, point2f lowerBound, point2f size, waypoint ***bufferPtr)
{
    if (level->children[0])
    {
        u08 toSearch[4] = {1, 1, 1, 1};
#define epsilon 0.001f
        ForLoop(4)
        {
            // bottom left
            waypoint_quadtree_level *child = level->children[index];
            point2f insertSize = {lowerBound.x - child->lowerBound.x, lowerBound.y - child->lowerBound.y};
            if (insertSize.x >= 0 && insertSize.x < child->size && insertSize.y >= 0 && insertSize.y < child->size)
            {
                point2f recSize = {my_Min(size.x, child->size - insertSize.x - epsilon), my_Min(size.y, child->size - insertSize.y - epsilon)};
                GetWaypointsWithinRectange(child, lowerBound, recSize, bufferPtr);
                toSearch[index] = 0;
                break;
            }
        }
        ForLoop(4)
        {
            if (toSearch[index])
            {
                // bottom right
                waypoint_quadtree_level *child = level->children[index];
                point2f insertSize = {lowerBound.x + size.x - child->lowerBound.x, lowerBound.y - child->lowerBound.y};
                if (insertSize.x >= 0 && insertSize.x < child->size && insertSize.y >= 0 && insertSize.y < child->size)
                {
                    point2f recSize = {my_Min(size.x, insertSize.x), my_Min(size.y, child->size - insertSize.y - epsilon)};
                    GetWaypointsWithinRectange(child, {child->lowerBound.x + insertSize.x - recSize.x, lowerBound.y}, recSize, bufferPtr);
                    toSearch[index] = 0;
                    break;
                }
            }
        }
        ForLoop(4)
        {
            if (toSearch[index])
            {
                // top left
                waypoint_quadtree_level *child = level->children[index];
                point2f insertSize = {lowerBound.x - child->lowerBound.x, lowerBound.y + size.y - child->lowerBound.y};
                if (insertSize.x >= 0 && insertSize.x < child->size && insertSize.y >= 0 && insertSize.y < child->size)
                {
                    point2f recSize = {my_Min(size.x, child->size - insertSize.x - epsilon), my_Min(size.y, insertSize.y)};
                    GetWaypointsWithinRectange(child, {lowerBound.x, child->lowerBound.y + insertSize.y - recSize.y}, recSize, bufferPtr);
                    toSearch[index] = 0;
                    break;
                }
            }
        }
        ForLoop(4)
        {
            if (toSearch[index])
            {
                // top right
                waypoint_quadtree_level *child = level->children[index];
                point2f insertSize = {lowerBound.x + size.x - child->lowerBound.x, lowerBound.y + size.y - child->lowerBound.y};
                if (insertSize.x >= 0 && insertSize.x < child->size && insertSize.y >= 0 && insertSize.y < child->size)
                {
                    point2f recSize = {my_Min(size.x, insertSize.x), my_Min(size.y, insertSize.y)};
                    GetWaypointsWithinRectange(child, {child->lowerBound.x + insertSize.x - recSize.x, child->lowerBound.y + insertSize.y - recSize.y}, recSize, bufferPtr);
#ifdef Internal
                    toSearch[index] = 0;
#endif
                    break;
                }
            }
        }
   
#ifdef Internal
        ForLoop(4)
        {
            if (!toSearch[index]) level->children[index]->show = 1;
        }
#endif
    
    }
    else
    {
        TraverseLinkedList(level->headNode.next, waypoint_quadtree_node)
        {
            **bufferPtr = node->wayp;
            ++(*bufferPtr);
        }
    }
}

#ifdef Internal
global_function
void
TurnOffDrawingForQuadTreeLevel(waypoint_quadtree_level *level = 0)
{
    if (!level) level = Waypoint_Editor->quadtree;
    level->show = 0;
    ForLoop(4)
    {
        if (level->children[index]) TurnOffDrawingForQuadTreeLevel(level->children[index]);
    }
}
#endif

global_function
void
GetWaypointsWithinSquare(point2f lowerBound, f32 size, waypoint ***bufferPtr
#ifdef Internal        
        , u32 reset = 1)
#else
        )
#endif
{
#ifdef Internal
    if (reset) TurnOffDrawingForQuadTreeLevel();
#endif
    GetWaypointsWithinRectange(Waypoint_Editor->quadtree, lowerBound, {size, size}, bufferPtr);
}

global_function
void
GetWaypointsWithinRectange(point2f lowerBound, point2f size, waypoint ***bufferPtr
#ifdef Internal        
        , u32 reset)
#else
        )
#endif
{
#ifdef Internal
    if (reset) TurnOffDrawingForQuadTreeLevel();
#endif
    GetWaypointsWithinRectange(Waypoint_Editor->quadtree, lowerBound, size, bufferPtr);
}

global_function
waypoint_quadtree_level *
GetWaypointQuadTreeLevel(point2f coords)
{
    coords = {my_Max(my_Min(coords.x, 0.499f), -0.5f), my_Max(my_Min(coords.y, 0.499f), -0.5f)};
    
    waypoint_quadtree_level *level = Waypoint_Editor->quadtree;

    while (level->children[0])
    {
#ifdef DEBUG
        u32 found = 0;
#endif
        ForLoop(4)
        {
            point2f insertSize = {coords.x - level->children[index]->lowerBound.x, coords.y - level->children[index]->lowerBound.y};
            f32 size = level->children[index]->size;
            if (insertSize.x >= 0 && insertSize.x < size && insertSize.y >= 0 && insertSize.y < size)
            {
                level = level->children[index];
                
#ifdef DEBUG
                found = 1;
#endif
                
                break;
            }
        }

#ifdef DEBUG
        if (!found)
        {
            Assert(found);
        }
#endif

    }

    return(level);
}

global_function
void
AddWayPoint(point2f coords)
{
    u32 nFree = Waypoints_Stack_Size - Waypoint_Editor->nWaypointsActive;

    if (nFree)
    {
        waypoint *wayp = Waypoint_Editor->freeWaypoints.next;
        Waypoint_Editor->freeWaypoints.next = wayp->next;
        if (Waypoint_Editor->freeWaypoints.next) Waypoint_Editor->freeWaypoints.next->prev = &Waypoint_Editor->freeWaypoints;

        wayp->next = Waypoint_Editor->activeWaypoints.next;
        if (Waypoint_Editor->activeWaypoints.next) Waypoint_Editor->activeWaypoints.next->prev = wayp;
        Waypoint_Editor->activeWaypoints.next = wayp;
        wayp->prev = &Waypoint_Editor->activeWaypoints;
        wayp->index = wayp->next ? wayp->next->index + 1 : 0;

        wayp->coords = coords;
        wayp->z = Camera_Position.z;
        ++Waypoint_Editor->nWaypointsActive;

        waypoint_quadtree_level *level = GetWaypointQuadTreeLevel(wayp->coords);
        
        waypoint_quadtree_node *node = Waypoint_Editor->freeNodes.next;
        if (node->next) node->next->prev = &Waypoint_Editor->freeNodes;
        Waypoint_Editor->freeNodes.next = node->next;

        if (level->headNode.next) level->headNode.next->prev = node;
        node->next = level->headNode.next;
        node->prev = &level->headNode;
        level->headNode.next = node;

        wayp->node = node;
        node->wayp = wayp;
    }
}

global_function
void
RemoveWayPoint(waypoint *wayp)
{
    switch (Waypoint_Editor->nWaypointsActive)
    {
        case 0:
            wayp = 0;
            break;

        case 1:
            Waypoint_Editor->activeWaypoints.next = 0;
            Waypoint_Editor->nWaypointsActive = 0;
            break;

        default:
            if (wayp->next) wayp->next->prev = wayp->prev;
            if (wayp->prev) wayp->prev->next = wayp->next;
            --Waypoint_Editor->nWaypointsActive;
    }

    if (wayp)
    {
        wayp->next = Waypoint_Editor->freeWaypoints.next;
        if (Waypoint_Editor->freeWaypoints.next) Waypoint_Editor->freeWaypoints.next->prev = wayp;
        Waypoint_Editor->freeWaypoints.next = wayp;
        wayp->prev = &Waypoint_Editor->freeWaypoints;

        waypoint_quadtree_node *node = wayp->node;

        if (node->next) node->next->prev = node->prev;
        if (node->prev) node->prev->next = node->next;
        node->prev = 0;

        if (Waypoint_Editor->freeNodes.next) Waypoint_Editor->freeNodes.next->prev = node;
        node->next = Waypoint_Editor->freeNodes.next;
        Waypoint_Editor->freeNodes.next = node;
        node->prev = &Waypoint_Editor->freeNodes;
    }
}

global_function
void
UpdateWayPoint(waypoint *wayp, point2f coords)
{
    waypoint_quadtree_node *node = wayp->node;

    if (node->next) node->next->prev = node->prev;
    if (node->prev) node->prev->next = node->next;
    node->prev = 0;

    wayp->coords = coords;

    waypoint_quadtree_level *level = GetWaypointQuadTreeLevel(wayp->coords);

    if (level->headNode.next) level->headNode.next->prev = node;
    node->next = level->headNode.next;
    node->prev = &level->headNode;
    level->headNode.next = node;

    wayp->node = node;
    node->wayp = wayp;
}

global_function
void
MouseMove(GLFWwindow* window, f64 x, f64 y)
{
    if (Loading || auto_sort_state || auto_cut_state)
    {
        return;
    }

    (void)window;

    u32 redisplay = 0;

    if (UI_On)
    {
    }
    else // ui_on == false
    {
        if (Edit_Mode)
        {
            static s32 netDelta = 0;

            s32 w, h;
            glfwGetWindowSize(window, &w, &h);
            f32 height = (f32)h;
            f32 width = (f32)w;

            f32 factor1 = 1.0f / (2.0f * Camera_Position.z);
            f32 factor2 = 2.0f / height;
            f32 factor3 = width * 0.5f;

            f32 wx = (factor1 * factor2 * ((f32)x - factor3)) + Camera_Position.x;
            f32 wy = (-factor1 * (1.0f - (factor2 * (f32)y))) - Camera_Position.y;

            wx = my_Max(-0.5f, my_Min(0.5f, wx));
            wy = my_Max(-0.5f, my_Min(0.5f, wy));

            u32 nPixels = Number_of_Pixels_1D;

            u32 pixel1 = (u32)((f64)nPixels * (0.5 + (f64)wx));
            u32 pixel2 = (u32)((f64)nPixels * (0.5 + (f64)wy));
            
            // 防止在尾部溢出
            pixel1 = my_Max(0, my_Min(nPixels - 1, pixel1));
            pixel2 = my_Max(0, my_Min(nPixels - 1, pixel2));

            u32 contig = Map_State->contigIds[pixel1]; 

            if (!Edit_Pixels.editing && !Edit_Pixels.selecting && Map_State->contigIds[pixel2] != contig)
            {
                u32 testPixel = pixel1;
                u32 testContig = contig;
                while (testContig == contig) // move testContig to the 1 before the first or 1 after the last pixel of the contig
                {

                    if (pixel1 < pixel2) 
                    {
                        testPixel ++ ;
                    }
                    else
                    {
                        testPixel -- ;
                    }

                    testContig = Map_State->contigIds[testPixel];

                    if (testPixel == 0 || testPixel >= (nPixels - 1)) break;

                    // testContig = pixel1 < pixel2 ? Map_State->contigIds[++testPixel] : Map_State->contigIds[--testPixel];
                    // if (testPixel == 0 || testPixel >= (nPixels - 1)) break;
                }   
                if (Map_State->contigIds[testPixel] == contig)
                {
                    pixel2 = testPixel;
                }
                else
                {
                    pixel2 = pixel1 < pixel2 ? testPixel - 1 : testPixel + 1;
                }
            }

            
            if (Edit_Pixels.selecting) // select the contig
            {
                Edit_Pixels.selectPixels.x = my_Max(pixel1, my_Max(Edit_Pixels.selectPixels.x, Edit_Pixels.selectPixels.y)); // used to select multiple contigs with pressing space and dragging the mouse
                // Edit_Pixels.selectPixels.x = pixel1;
                while(  Edit_Pixels.selectPixels.x < (Number_of_Pixels_1D - 1) &&
                        ((Edit_Pixels.scaffSelecting && Map_State->scaffIds[Edit_Pixels.selectPixels.x] && Map_State->scaffIds[Edit_Pixels.selectPixels.x] == Map_State->scaffIds[1 + Edit_Pixels.selectPixels.x] ) || 
                          Map_State->contigIds[Edit_Pixels.selectPixels.x] == Map_State->contigIds[1 + Edit_Pixels.selectPixels.x]))
                {
                    ++Edit_Pixels.selectPixels.x;
                }

                Edit_Pixels.selectPixels.y = my_Min(pixel1, my_Min(Edit_Pixels.selectPixels.x, Edit_Pixels.selectPixels.y));
                // Edit_Pixels.selectPixels.y = pixel1;
                while(  Edit_Pixels.selectPixels.y > 0 &&
                        ((Edit_Pixels.scaffSelecting && Map_State->scaffIds[Edit_Pixels.selectPixels.y] && Map_State->scaffIds[Edit_Pixels.selectPixels.y] == Map_State->scaffIds[Edit_Pixels.selectPixels.y - 1]) || Map_State->contigIds[Edit_Pixels.selectPixels.y] == Map_State->contigIds[Edit_Pixels.selectPixels.y - 1]))
                {
                    --Edit_Pixels.selectPixels.y;
                }

                pixel1 = Edit_Pixels.selectPixels.x;
                pixel2 = Edit_Pixels.selectPixels.y;
            }

            if (Edit_Pixels.editing)
            {
                u32 midPixel = (pixel1 + pixel2) >> 1;
                u32 oldMidPixel = (Edit_Pixels.pixels.x + Edit_Pixels.pixels.y) >> 1;
                s32 diff = (s32)midPixel - (s32)oldMidPixel;

                u32 forward = diff > 0;
                
                s32 newX = (s32)Edit_Pixels.pixels.x + diff;
                newX = my_Max(0, my_Min((s32)nPixels - 1, newX));
                s32 newY = (s32)Edit_Pixels.pixels.y + diff;
                newY = my_Max(0, my_Min((s32)nPixels - 1, newY));

                s32 diffx = newX - (s32)Edit_Pixels.pixels.x;
                s32 diffy = newY - (s32)Edit_Pixels.pixels.y;

                diff = forward ? my_Min(diffx, diffy) : my_Max(diffx, diffy);
                
                //newX = (s32)Edit_Pixels.pixels.x + diff;
                //newY = (s32)Edit_Pixels.pixels.y + diff;
                
                diff = RearrangeMap(Edit_Pixels.pixels.x, Edit_Pixels.pixels.y, diff, Edit_Pixels.snap);
                netDelta += diff;

                newX = (s32)Edit_Pixels.pixels.x + diff;
                newY = (s32)Edit_Pixels.pixels.y + diff;
                
                Edit_Pixels.pixels.x = (u32)newX;
                Edit_Pixels.pixels.y = (u32)newY;

                Edit_Pixels.worldCoords.x = (f32)(((f64)((2 * Edit_Pixels.pixels.x) + 1)) / ((f64)(2 * nPixels))) - 0.5f;
                Edit_Pixels.worldCoords.y = (f32)(((f64)((2 * Edit_Pixels.pixels.y) + 1)) / ((f64)(2 * nPixels))) - 0.5f;
            }
            else  // edit_pixels.editing == 0
            {
                if (netDelta || Global_Edit_Invert_Flag)
                {
                    AddMapEdit(netDelta, Edit_Pixels.pixels, Global_Edit_Invert_Flag);
                    netDelta = 0;
                }
                
                wx = (f32)(((f64)((2 * pixel1) + 1)) / ((f64)(2 * nPixels))) - 0.5f;
                wy = (f32)(((f64)((2 * pixel2) + 1)) / ((f64)(2 * nPixels))) - 0.5f;

                Edit_Pixels.pixels.x = pixel1;
                Edit_Pixels.pixels.y = pixel2;
                Edit_Pixels.worldCoords.x = wx;
                Edit_Pixels.worldCoords.y = wy;
                
                Global_Edit_Invert_Flag = 0;
            }

            redisplay = 1;
        }
        else if (
            Tool_Tip->on || 
            Waypoint_Edit_Mode || 
            Scaff_Edit_Mode || 
            MetaData_Edit_Mode || 
            Select_Sort_Area_Mode)
        {
            s32 w, h;
            glfwGetWindowSize(window, &w, &h);
            f32 height = (f32)h;
            f32 width = (f32)w;

            f32 factor1 = 1.0f / (2.0f * Camera_Position.z);
            f32 factor2 = 2.0f / height;
            f32 factor3 = width * 0.5f;

            f32 wx = (factor1 * factor2 * ((f32)x - factor3)) + Camera_Position.x;
            f32 wy = (-factor1 * (1.0f - (factor2 * (f32)y))) - Camera_Position.y;

            wx = my_Max(-0.5f, my_Min(0.5f, wx));
            wy = my_Max(-0.5f, my_Min(0.5f, wy));

            u32 nPixels = Number_of_Textures_1D * Texture_Resolution;

            u32 pixel1 = my_Min((u32)((f64)nPixels * (0.5 + (f64)wx)), (nPixels - 1));
            u32 pixel2 = my_Min((u32)((f64)nPixels * (0.5 + (f64)wy)), (nPixels - 1));

            Tool_Tip_Move.pixels.x = pixel1;
            Tool_Tip_Move.pixels.y = pixel2;
            Tool_Tip_Move.worldCoords.x = wx;
            Tool_Tip_Move.worldCoords.y = wy;

            if (Waypoint_Edit_Mode)
            {
                Selected_Waypoint = 0;
                f32 selectDis = Waypoint_Select_Distance / (height * Camera_Position.z);
                f32 closestDistanceSq = selectDis * selectDis;

                waypoint **searchBuffer = PushArray(Working_Set, waypoint*, Waypoints_Stack_Size);
                waypoint **bufferEnd = searchBuffer;
                GetWaypointsWithinSquare( // add waypoints within the square to the buffer
                    {Tool_Tip_Move.worldCoords.x - selectDis, Tool_Tip_Move.worldCoords.y - selectDis},
                    2.0f * selectDis, 
                    &bufferEnd);
                for (   waypoint **waypPtr = searchBuffer;
                        waypPtr != bufferEnd;
                        ++waypPtr ) { // make sure select the closest waypoint to the mouse
                    waypoint *wayp = *waypPtr;

                    f32 dx = Tool_Tip_Move.worldCoords.x - wayp->coords.x;
                    f32 dy = Tool_Tip_Move.worldCoords.y - wayp->coords.y;
                    f32 disSq = (dx * dx) + (dy * dy);

                    if (disSq < closestDistanceSq)
                    {
                        closestDistanceSq = disSq;
                        Selected_Waypoint = wayp;
                    }
                }

                FreeLastPush(Working_Set); // searchBuffer
            }
            else if (Scaff_Edit_Mode && Scaff_Painting_Flag)
            {
                if (Scaff_Painting_Flag == 1)
                {
                    if (!Scaff_Painting_Id)
                    {
                        if (Map_State->scaffIds[Tool_Tip_Move.pixels.x])
                        {
                            Scaff_Painting_Id = Map_State->scaffIds[Tool_Tip_Move.pixels.x];
                        }
                        else
                        {
                            u32 max = 0;
                            ForLoop(Number_of_Pixels_1D) max = my_Max(max, Map_State->scaffIds[index]);
                            Scaff_Painting_Id = max + 1;
                        }
                    }
                }
                else Scaff_Painting_Id = 0;

                if (Map_State->scaffIds[Tool_Tip_Move.pixels.x] != Scaff_Painting_Id || (Scaff_FF_Flag & 1))
                {
                    u32 pixel = Tool_Tip_Move.pixels.x;

                    u32 currScaffId = Map_State->scaffIds[pixel];
                    u32 contigId = Map_State->contigIds[pixel];
                    Map_State->scaffIds[pixel] = Scaff_Painting_Id;

                    // set the pixels with same contig_id into the same scaff
                    u32 testPixel = pixel;
                    while (testPixel && (Map_State->contigIds[testPixel - 1] == contigId)) Map_State->scaffIds[--testPixel] = Scaff_Painting_Id;
                    testPixel = pixel;
                    while ((testPixel < (Number_of_Pixels_1D - 1)) && (Map_State->contigIds[testPixel + 1] == contigId)) Map_State->scaffIds[++testPixel] = Scaff_Painting_Id;

                    if (Scaff_FF_Flag & 1)
                    {
                        ++testPixel;
                        while ((testPixel < Number_of_Pixels_1D) && ((Scaff_FF_Flag & 2) || (Map_State->scaffIds[testPixel] == (Scaff_Painting_Flag == 1 ? 0 : currScaffId)))) Map_State->scaffIds[testPixel++] = Scaff_Painting_Id;
                    }
                }

                UpdateContigsFromMapState();
            }
            else if (Select_Sort_Area_Mode)
            {   
                auto_curation_state.update_sort_area(Tool_Tip_Move.pixels.x, Map_State, Number_of_Pixels_1D);
            }
            else if (
                MetaData_Edit_Mode && 
                MetaData_Edit_State && 
                strlen((const char *)Meta_Data->tags[MetaData_Active_Tag])
            )
            {
                u32 pixel = Tool_Tip_Move.pixels.x;
                u32 contigId = Map_State->contigIds[pixel];

                if (MetaData_Edit_State == 1) 
                    Map_State->metaDataFlags[pixel] |=  (1 << MetaData_Active_Tag); // set the active tag
                else 
                    Map_State->metaDataFlags[pixel] &= ~(1 << MetaData_Active_Tag); // clear the active tag
                
                u32 testPixel = pixel;
                while (testPixel && (Map_State->contigIds[testPixel - 1] == contigId))
                {
                    if (MetaData_Edit_State == 1) 
                        Map_State->metaDataFlags[--testPixel] |=  (1 << MetaData_Active_Tag);
                    else 
                        Map_State->metaDataFlags[--testPixel] &= ~(1 << MetaData_Active_Tag);
                }

                testPixel = pixel;
                while ((testPixel < (Number_of_Pixels_1D - 1)) && (Map_State->contigIds[testPixel + 1] == contigId))
                {
                    if (MetaData_Edit_State == 1) 
                        Map_State->metaDataFlags[++testPixel] |=  (1 << MetaData_Active_Tag);
                    else 
                        Map_State->metaDataFlags[++testPixel] &= ~(1 << MetaData_Active_Tag);
                }

                UpdateContigsFromMapState();
            }

            redisplay = 1;
        }

        if (Mouse_Move.x >= 0.0)
        {
            s32 w, h;
            glfwGetWindowSize(window, &w, &h);
            f32 height = (f32)h;

            f32 factor = 1.0f / (height * Camera_Position.z);
            f32 dx = (f32)(Mouse_Move.x - x) * factor;
            f32 dy = (f32)(y - Mouse_Move.y) * factor;

            Camera_Position.x += dx;
            Camera_Position.y += dy;
            ClampCamera();

            Mouse_Move.x = x;
            Mouse_Move.y = y;

            redisplay = 1;
        }
    }

    if (redisplay)
    {
        Redisplay = 1;
    }
}


global_variable
u08
Grey_Haplotigs = 1;

global_variable
GreyOutSettings* Grey_Out_Settings = nullptr;

global_variable
UserProfileSettings* user_profile_settings_ptr = nullptr;

global_variable
u08
Deferred_Close_UI = 0;

global_function
void
Mouse(GLFWwindow* window, s32 button, s32 action, s32 mods)
{
    s32 primaryMouse = user_profile_settings_ptr->invert_mouse ? GLFW_MOUSE_BUTTON_RIGHT : GLFW_MOUSE_BUTTON_LEFT;
    s32 secondaryMouse = user_profile_settings_ptr->invert_mouse ? GLFW_MOUSE_BUTTON_LEFT : GLFW_MOUSE_BUTTON_RIGHT;    
    
    if (Loading || auto_sort_state)
    {
        return;
    }
    
    (void)mods;

    f64 x, y;
    glfwGetCursorPos(window, &x, &y);

    if (UI_On)
    {
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        {
            Deferred_Close_UI = 1;
        }
    }
    else // UI not on 
    {
        if (button == primaryMouse && Edit_Mode && action == GLFW_PRESS)
        {
            Edit_Pixels.editing = !Edit_Pixels.editing;
            MouseMove(window, x, y);
            if (!Edit_Pixels.editing) UpdateScaffolds();
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && Edit_Mode && action == GLFW_RELEASE && !Edit_Pixels.editing)
        {
            Edit_Pixels.editing = 1;
            Edit_Pixels.selecting = 0;
            MouseMove(window, x, y);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && Edit_Mode && action == GLFW_PRESS && !Edit_Pixels.editing)
        {
            Edit_Pixels.selecting = 1;
            Edit_Pixels.selectPixels = Edit_Pixels.pixels;
            MouseMove(window, x, y);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && Edit_Mode && Edit_Pixels.editing && action == GLFW_PRESS)
        {
            InvertMap(Edit_Pixels.pixels.x, Edit_Pixels.pixels.y);
            Global_Edit_Invert_Flag = !Global_Edit_Invert_Flag;
            UpdateContigsFromMapState();

            Redisplay = 1;
        }
        else if (button == primaryMouse && Waypoint_Edit_Mode && action == GLFW_PRESS)
        {
            AddWayPoint(Tool_Tip_Move.worldCoords);
            MouseMove(window, x, y);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && Waypoint_Edit_Mode && action == GLFW_PRESS)
        {
            if (Selected_Waypoint)
            {
                RemoveWayPoint(Selected_Waypoint);
                MouseMove(window, x, y);
            }
        }
        else if (button == primaryMouse && Scaff_Edit_Mode && action == GLFW_PRESS)
        {
            Scaff_Painting_Flag = 1;
            MouseMove(window, x, y);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && Scaff_Edit_Mode && action == GLFW_PRESS)
        {
            Scaff_Painting_Flag = 2;
            MouseMove(window, x, y);
        }
        else if ((button == GLFW_MOUSE_BUTTON_MIDDLE || button == primaryMouse) && Scaff_Edit_Mode && action == GLFW_RELEASE)
        {
            Scaff_Painting_Flag = 0;
            Scaff_Painting_Id = 0;
            MouseMove(window, x, y);
            UpdateScaffolds();
        }
        else if (button == primaryMouse && Select_Sort_Area_Mode && action  == GLFW_PRESS)
        {
            auto_curation_state.select_mode = 1;
            MouseMove(window, x, y);
        }
        else if (button == primaryMouse && Select_Sort_Area_Mode && action == GLFW_RELEASE)
        {
            auto_curation_state.select_mode = 0;
            MouseMove(window, x, y);
        }
        else if (button == primaryMouse && MetaData_Edit_Mode && action == GLFW_PRESS)
        {
            MetaData_Edit_State = 1;
            MouseMove(window, x, y);
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && MetaData_Edit_Mode && action == GLFW_PRESS)
        {
            MetaData_Edit_State = 2;
            MouseMove(window, x, y);
        }
        else if ((button == GLFW_MOUSE_BUTTON_MIDDLE || button == primaryMouse) && MetaData_Edit_Mode && action == GLFW_RELEASE)
        {
            MetaData_Edit_State = 0;
            MouseMove(window, x, y);
        }
        else if (button == secondaryMouse)
        {
            if (action == GLFW_PRESS)
            {
                Mouse_Move.x = x;
                Mouse_Move.y = y;
            }
            else
            {
                Mouse_Move.x = Mouse_Move.y = -1.0;
            }
        }
        else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        {
            UI_On = !UI_On;
            Mouse_Move.x = Mouse_Move.y = -1;
            Redisplay = 1;
            ++NK_Device->lastContextMemory[0];
        }
    }
}

global_variable
struct nk_vec2
NK_Scroll;

global_function
void
Scroll(GLFWwindow* window, f64 x, f64 y)
{
    if (Loading || auto_sort_state)
    {
        return;
    }
    
    if (UI_On)
    {
        NK_Scroll.y = (f32)y;
        NK_Scroll.x = (f32)x;
    }
    else
    {
        if (y != 0.0)
        {
            ZoomCamera(my_Max(my_Min((f32)y, 0.01f), -0.01f));
            Redisplay = 1;
        }

        if (Edit_Mode || Tool_Tip->on || Waypoint_Edit_Mode)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
}

global_variable
u64
Total_Genome_Length;

global_function
void
Setup();

global_function
u32
SubDivideScaleBar(f32 left, f32 right, f32 middle, f32 bpPerPixel, f32 offset)
{
    u32 result = 0;

    if (left < right)
    {
        f32 length = right - left;
        f32 half = length * 0.5f;
        char buff[16];
        stbsp_snprintf(buff, 16, "%$.2f", (f64)(offset + (half * bpPerPixel)));
        f32 width = fonsTextBounds(FontStash_Context, middle, 0.0, buff, 0, NULL);
        f32 halfWidth = 0.5f * width;

        if ((middle + halfWidth) < right && (middle - halfWidth) > left)
        {
            u32 leftResult = SubDivideScaleBar(left, middle - halfWidth, (left + middle) * 0.5f, bpPerPixel, offset);
            u32 rightResult = SubDivideScaleBar(middle + halfWidth, right, (right + middle) * 0.5f, bpPerPixel, offset);
            result = 1 + my_Min(leftResult, rightResult);   
        }
    }

    return(result);
}

global_variable
u32
File_Loaded = 0;

global_variable
nk_color
Theme_Colour;

#ifdef Internal
global_function
void
DrawQuadTreeLevel (u32 *ptr, waypoint_quadtree_level *level, vertex *vert, f32 lineWidth, u32 n = 0)
{
    f32 colour[5][4] = {{1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f, 1.0f}};
    f32 *col = (f32 *)colour[n%5];

    glUniform4fv(Flat_Shader->colorLocation, 1, col);
    
    if (level->show)
    {
        vert[0].x = level->lowerBound.x;
        vert[0].y = -level->lowerBound.y;
        vert[1].x = level->lowerBound.x;
        vert[1].y = -level->lowerBound.y - level->size;
        vert[2].x = level->lowerBound.x + lineWidth;
        vert[2].y = -level->lowerBound.y - level->size;
        vert[3].x = level->lowerBound.x + lineWidth;
        vert[3].y = -level->lowerBound.y;

        glBindBuffer(GL_ARRAY_BUFFER, QuadTree_Data->vbos[*ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(QuadTree_Data->vaos[(*ptr)++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        vert[0].x = level->lowerBound.x;
        vert[0].y = -level->lowerBound.y - level->size;
        vert[1].x = level->lowerBound.x + level->size;
        vert[1].y = -level->lowerBound.y - level->size;
        vert[2].x = level->lowerBound.x + level->size;
        vert[2].y = -level->lowerBound.y - level->size + lineWidth;
        vert[3].x = level->lowerBound.x;
        vert[3].y = -level->lowerBound.y - level->size + lineWidth;

        glBindBuffer(GL_ARRAY_BUFFER, QuadTree_Data->vbos[*ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(QuadTree_Data->vaos[(*ptr)++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        vert[0].x = level->lowerBound.x + level->size - lineWidth;
        vert[0].y = -level->lowerBound.y;
        vert[1].x = level->lowerBound.x + level->size - lineWidth;
        vert[1].y = -level->lowerBound.y - level->size;
        vert[2].x = level->lowerBound.x + level->size;
        vert[2].y = -level->lowerBound.y - level->size;
        vert[3].x = level->lowerBound.x + level->size;
        vert[3].y = -level->lowerBound.y;

        glBindBuffer(GL_ARRAY_BUFFER, QuadTree_Data->vbos[*ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(QuadTree_Data->vaos[(*ptr)++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        vert[0].x = level->lowerBound.x;
        vert[0].y = -level->lowerBound.y;
        vert[1].x = level->lowerBound.x;
        vert[1].y = -level->lowerBound.y - lineWidth;
        vert[2].x = level->lowerBound.x + level->size;
        vert[2].y = -level->lowerBound.y - lineWidth;
        vert[3].x = level->lowerBound.x + level->size;
        vert[3].y = -level->lowerBound.y;

        glBindBuffer(GL_ARRAY_BUFFER, QuadTree_Data->vbos[*ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(QuadTree_Data->vaos[(*ptr)++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
    }

    if (level->children[0])
    {
        ForLoop(4)
        {
            DrawQuadTreeLevel(ptr, level->children[index], vert, lineWidth, n+1);
        }
    }
}
#endif

global_function
u32
FourFloatColorToU32(nk_colorf colour)
{
    return(glfonsRGBA((u08)(colour.r * 255.0f), (u08)(colour.g * 255.0f),
                (u08)(colour.b * 255.0f), (u08)(colour.a * 255.0f)));
}

global_function
u32
ThreeFloatColorToU32(nk_colorf colour)
{
    return(glfonsRGBA((u08)(colour.r * 255.0f), (u08)(colour.g * 255.0f),
                (u08)(colour.b * 255.0f), 255));
}

global_function
void
ColourGenerator(u32 index, f32 *rgb)
{
// #define RedFreq 1.666f
// #define GreenFreq 2.666f
// #define BlueFreq 3.666f

//     rgb[0] = 0.5f * (sinf((f32)index * RedFreq) + 1.0f);
//     rgb[1] = 0.5f * (sinf((f32)index * GreenFreq) + 1.0f);
//     rgb[2] = 0.5f * (sinf((f32)index * BlueFreq) + 1.0f);

    f32 RedFreq = meta_dataColors[meta_data_curcolorProfile][0];
    f32 GreenFreq = meta_dataColors[meta_data_curcolorProfile][1];
    f32 BlueFreq = meta_dataColors[meta_data_curcolorProfile][2];

    rgb[0] = 0.5f * (sinf((f32)index * RedFreq) + 1.0f);
    rgb[1] = 0.5f * (sinf((f32)index * GreenFreq) + 1.0f);
    rgb[2] = 0.5f * (sinf((f32)index * BlueFreq) + 1.0f);
}


void DrawOutlinedText(
    FONScontext *FontStash_Context, 
    nk_colorf *colour, 
    const char *text, 
    f32 x, 
    f32 y, 
    f32 offset = 1.0f, 
    bool outline_on = false)
{

    nk_colorf outlineColor;
    if (meta_outline->on == 1)
    {
        outlineColor = {0.0f, 0.0f, 0.0f, 1.0f}; // black
    }
    else
    {
        outlineColor = {1.0f, 1.0f, 1.0f, 1.0f}; // white 
    }

    if (outline_on) 
        outlineColor = {0.0f, 0.0f, 0.0f, 1.0f}; // black

    unsigned int outlineColorU32 = FourFloatColorToU32(outlineColor);
    unsigned int textColorU32 = FourFloatColorToU32(*colour);

    if (meta_outline->on > 0 || outline_on)
    {
        fonsSetColor(FontStash_Context, outlineColorU32);
        fonsDrawText(FontStash_Context, x - offset, y - offset, text, 0);
        fonsDrawText(FontStash_Context, x + offset, y - offset, text, 0);
        fonsDrawText(FontStash_Context, x - offset, y + offset, text, 0);
        fonsDrawText(FontStash_Context, x + offset, y + offset, text, 0);
    }

    // // Draw the original text on top
    fonsSetColor(FontStash_Context, textColorU32);
    fonsDrawText(FontStash_Context, x, y, text, 0);
}


global_variable
char
Extension_Magic_Bytes[][4] = 
{
    {'p', 's', 'g', 'h'}
};



global_variable
extension_sentinel
Extensions = {};

global_function
void
AddExtension(extension_node *node)
{
    node->next = 0;

    if (!Extensions.head)
    {
        Extensions.head = node;
        Extensions.tail = node;
    }
    else
    {
        Extensions.tail->next = node; // append the last to the end
        Extensions.tail = node;       // set the last to the new node just appended
    }
}

global_function
void
Render() {
    // Projection Matrix
    f32 width;
    f32 height;
    {
        glClear(GL_COLOR_BUFFER_BIT);
        // glClearColor(0.2f, 0.6f, 0.4f, 1.0f); // classic background color
        glClearColor(bgcolor[active_bgcolor][0], bgcolor[active_bgcolor][1], bgcolor[active_bgcolor][2], bgcolor[active_bgcolor][3]);

        s32 viewport[4];
        glGetIntegerv (GL_VIEWPORT, viewport);
        width = (f32)viewport[2];
        height = (f32)viewport[3];

        f32 mat[16];
        memset(mat, 0, sizeof(mat));
        mat[0] = 2.0f * Camera_Position.z * height / width;
        mat[5] = 2.0f * Camera_Position.z;
        mat[10] = -2.0f;
        mat[12] = -2.0f * height * Camera_Position.x * Camera_Position.z / width;
        mat[13] = -2.0f * Camera_Position.y * Camera_Position.z;
        mat[15] = 1.0f;

        /*
        template<typename T>
        GLM_FUNC_QUALIFIER mat<4, 4, T, defaultp> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar)
        {
            mat<4, 4, T, defaultp> Result(1);
            Result[0][0] = static_cast<T>(2) / (right - left);  // 0 
            Result[1][1] = static_cast<T>(2) / (top - bottom);  // 5
            Result[2][2] = - static_cast<T>(2) / (zFar - zNear);// 10
            Result[3][0] = - (right + left) / (right - left);   // 12 
            Result[3][1] = - (top + bottom) / (top - bottom);   // 13
            Result[3][2] = - (zFar + zNear) / (zFar - zNear);   // 14
            return Result;
        }
        glm::mat4 projection_mat = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
        */

        // apply the transformation
        glUseProgram(Contact_Matrix->shaderProgram);
        glUniformMatrix4fv(Contact_Matrix->matLocation, 1, GL_FALSE, mat);
        glUseProgram(Flat_Shader->shaderProgram);
        glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, mat);

        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
                case extension_graph:
                    {
                        graph *gph = (graph *)node->extension;
                        glUseProgram(gph->shader->shaderProgram);
                        glUniformMatrix4fv(gph->shader->matLocation, 1, GL_FALSE, mat);
                    }
                    break;
            }
        }
    }

    // Textures
    if (File_Loaded)
    {
        glUseProgram(Contact_Matrix->shaderProgram);
        glBindTexture(GL_TEXTURE_2D_ARRAY, Contact_Matrix->textures);
        u32 ptr = 0;
        while (ptr < Number_of_Textures_1D * Number_of_Textures_1D)
        {
            glBindVertexArray(Contact_Matrix->vaos[ptr++]); // bind the vertices and then draw the quad
            glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
        }
    }

    #ifdef Internal
    if (File_Loaded && Tiles->on)
    {
        glUseProgram(Flat_Shader->shaderProgram);
        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Tiles->bg);

        u32 ptr = 0;
        vertex vert[4];

        f32 lineWidth = 0.001f / sqrtf(Camera_Position.z);
        f32 position = 0.0f;
        f32 spacing = 1.0f / (f32)Number_of_Textures_1D;

        vert[0].x = -0.5f;
        vert[0].y = -0.5f;
        vert[1].x = lineWidth - 0.5f;
        vert[1].y = -0.5f;
        vert[2].x = lineWidth - 0.5f;
        vert[2].y = 0.5f;
        vert[3].x = -0.5f;
        vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        f32 x = -0.5f;
        ForLoop(Number_of_Textures_1D - 1)
        {
            position += spacing;
            f32 px = x + lineWidth;
            x = position - (0.5f * (lineWidth + 1.0f));

            if (x > px)
            {
                vert[0].x = x;
                vert[0].y = -0.5f;
                vert[1].x = x + lineWidth;
                vert[1].y = -0.5f;
                vert[2].x = x + lineWidth;
                vert[2].y = 0.5f;
                vert[3].x = x;
                vert[3].y = 0.5f;

                glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
        }

        vert[0].x = 0.5f - lineWidth;
        vert[0].y = -0.5f;
        vert[1].x = 0.5f;
        vert[1].y = -0.5f;
        vert[2].x = 0.5f;
        vert[2].y = 0.5f;
        vert[3].x = 0.5f - lineWidth;
        vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        position = 0.0f;

        vert[0].x = -0.5f;
        vert[0].y = 0.5f - lineWidth;
        vert[1].x = 0.5f;
        vert[1].y = 0.5f - lineWidth;
        vert[2].x = 0.5f;
        vert[2].y = 0.5f;
        vert[3].x = -0.5f;
        vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        f32 y = 0.5f;
        ForLoop(Number_of_Textures_1D - 1)
        {
            position += spacing;
            f32 py = y - lineWidth;
            y = 1.0f - position + (0.5f * (lineWidth - 1.0f));

            if (y < py)
            {
                vert[0].x = -0.5f;
                vert[0].y = y - lineWidth;
                vert[1].x = 0.5f;
                vert[1].y = y - lineWidth;
                vert[2].x = 0.5f;
                vert[2].y = y;
                vert[3].x = -0.5f;
                vert[3].y = y;

                glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
        }

        vert[0].x = -0.5f;
        vert[0].y = -0.5f;
        vert[1].x = 0.5f;
        vert[1].y = -0.5f;
        vert[2].x = 0.5f;
        vert[2].y = lineWidth - 0.5f;
        vert[3].x = -0.5f;
        vert[3].y = lineWidth - 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Texture_Tile_Grid->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Texture_Tile_Grid->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
    }

    // Quad Trees
    //if (Waypoint_Edit_Mode)
    {
        glUseProgram(Flat_Shader->shaderProgram);
        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&QuadTrees->bg);

        u32 ptr = 0;
        vertex vert[4];

        f32 lineWidth = 0.001f / sqrtf(Camera_Position.z);

        DrawQuadTreeLevel(&ptr, Waypoint_Editor->quadtree, vert, lineWidth);
    }
    #endif
   
    // Extensions
    u08 graphNamesOn = 0;
    {
        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
                case extension_graph:
                    {
                        graph *gph = (graph *)node->extension;

                        if (gph->on)
                        {
                            if (gph->nameOn) graphNamesOn = 1;

                            f32 factor1 = 1.0f / (2.0f * Camera_Position.z);
                            f32 factor2 = 2.0f / height;

                            f32 wy = (factor1 * (1.0f - (factor2 * (height - gph->base)))) + Camera_Position.y;

                            glUseProgram(gph->shader->shaderProgram);
                            glUniform4fv(gph->shader->colorLocation, 1, (GLfloat *)&gph->colour);
                            glUniform1f(gph->shader->yScaleLocation, gph->scale);
                            glUniform1f(gph->shader->yTopLocation, wy);
                            glUniform1f(gph->shader->lineSizeLocation, gph->lineSize / Camera_Position.z);

                            glBindBuffer(GL_ARRAY_BUFFER, gph->vbo);
                            glBindVertexArray(gph->vao);
                            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)Number_of_Pixels_1D);
                        }
                    }
                    break;
            }
        }
    }

    // Grid
    if (File_Loaded && Grid->on)
    {
        glUseProgram(Flat_Shader->shaderProgram);
        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Grid->bg);

        u32 ptr = 0;
        vertex vert[4];

        f32 lineWidth = Grid->size / sqrtf(Camera_Position.z);
        f32 position = 0.0f;

        // left border
        vert[0].x = -0.5f;            vert[0].y = -0.5f;
        vert[1].x = lineWidth - 0.5f; vert[1].y = -0.5f;
        vert[2].x = lineWidth - 0.5f; vert[2].y = 0.5f;
        vert[3].x = -0.5f;            vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Grid_Data->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        f32 x = -0.5f;
        ForLoop(Contigs->numberOfContigs - 1)
        {
            contig *cont = Contigs->contigs_arr + index;

            position += ((f32)cont->length / (f32)Number_of_Pixels_1D);
            f32 px = x + lineWidth;
            x = position - (0.5f * (lineWidth + 1.0f));

            if (x > px)
            {   
                // contig vertical line
                vert[0].x = x;             vert[0].y = -0.5f;
                vert[1].x = x + lineWidth; vert[1].y = -0.5f;
                vert[2].x = x + lineWidth; vert[2].y = 0.5f;
                vert[3].x = x;             vert[3].y = 0.5f;

                glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Grid_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
        }
        // right border
        vert[0].x = 0.5f - lineWidth; vert[0].y = -0.5f;
        vert[1].x = 0.5f;             vert[1].y = -0.5f;
        vert[2].x = 0.5f;             vert[2].y = 0.5f;
        vert[3].x = 0.5f - lineWidth; vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Grid_Data->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        position = 0.0f;
        // top border
        vert[0].x = -0.5f; vert[0].y = 0.5f - lineWidth;
        vert[1].x = 0.5f;  vert[1].y = 0.5f - lineWidth;
        vert[2].x = 0.5f;  vert[2].y = 0.5f;
        vert[3].x = -0.5f; vert[3].y = 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Grid_Data->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

        f32 y = 0.5f;
        ForLoop(Contigs->numberOfContigs - 1)
        {
            contig *cont = Contigs->contigs_arr + index;

            position += ((f32)cont->length / (f32)Number_of_Pixels_1D);
            f32 py = y - lineWidth;
            y = 1.0f - position + (0.5f * (lineWidth - 1.0f));

            if (y < py)
            {   
                // contig horizontal line
                vert[0].x = -0.5f; vert[0].y = y - lineWidth;
                vert[1].x = 0.5f;  vert[1].y = y - lineWidth;
                vert[2].x = 0.5f;  vert[2].y = y;
                vert[3].x = -0.5f; vert[3].y = y;

                glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Grid_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
        }
        // bottom border
        vert[0].x = -0.5f; vert[0].y = -0.5f;
        vert[1].x = 0.5f;  vert[1].y = -0.5f;
        vert[2].x = 0.5f;  vert[2].y = lineWidth - 0.5f;
        vert[3].x = -0.5f; vert[3].y = lineWidth - 0.5f;

        glBindBuffer(GL_ARRAY_BUFFER, Grid_Data->vbos[ptr]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
        glBindVertexArray(Grid_Data->vaos[ptr++]);
        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
    }

    // Contig Id Bars
    if (File_Loaded && Contig_Ids->on)
    {
        glUseProgram(Flat_Shader->shaderProgram);
        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Grid->bg);

        u32 ptr = 0;
        vertex vert[4];

        f32 barColour[4] = {1.0f, 1.0f, 1.0f, 1.0f};

        f32 lineWidth = Contig_Ids->size / sqrtf(Camera_Position.z);
        f32 position = 0.0f;

        f32 y = 0.5f;
        ForLoop(Contigs->numberOfContigs)
        {
            contig *cont = Contigs->contigs_arr + index;

            position += ((f32)cont->length / (f32)Number_of_Pixels_1D);
            f32 py = y - lineWidth;
            y = 1.0f - position + (0.5f * (lineWidth - 1.0f));

            if (y < py)
            {
                u32 invert = IsContigInverted(index);

                vert[0].x = -py; vert[0].y = invert ? y : (py + lineWidth);
                vert[1].x = -py; vert[1].y = invert ? (y - lineWidth) : py;
                vert[2].x = -y;  vert[2].y = invert ? (y - lineWidth) : py;
                vert[3].x = -y;  vert[3].y = invert ? y : (py + lineWidth);

                ColourGenerator((u32)cont->originalContigId, (f32 *)barColour);
                glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&barColour);

                glBindBuffer(GL_ARRAY_BUFFER, Contig_ColourBar_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Contig_ColourBar_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
        }
    }

    // Text / UI Rendering
    if (Contig_Name_Labels->on || Scale_Bars->on || Tool_Tip->on || UI_On || Loading || auto_sort_state || Edit_Mode || Waypoint_Edit_Mode || Waypoints_Always_Visible || Scaff_Edit_Mode || Scaffs_Always_Visible || MetaData_Edit_Mode || MetaData_Always_Visible || graphNamesOn)
    {
        f32 textNormalMat[16];
        f32 textRotMat[16];

        f32 w2 = width * 0.5f;
        f32 h2 = height * 0.5f;
        f32 hz = height * Camera_Position.z;

        // coverting model coords (world space) to screen coords
        auto ModelXToScreen = [hz, w2](f32 xin)->f32 { 
            return (xin - Camera_Position.x) * hz + w2; };
        auto ModelYToScreen = [hz, h2](f32 yin)->f32 { 
            return h2 - (yin - Camera_Position.y) * hz; };

        // Text Projection Matrix
        {
            memset(textNormalMat, 0, sizeof(textNormalMat));
            textNormalMat[0] = 2.0f / width;
            textNormalMat[5] = -2.0f / height;
            textNormalMat[10] = -1.0f;
            textNormalMat[12] = -1.0f;
            textNormalMat[13] = 1.0f;
            textNormalMat[15] = 1.0f;

            memset(textRotMat, 0, sizeof(textRotMat));
            textRotMat[4] = 2.0f / width;
            textRotMat[1] = 2.0f / height;
            textRotMat[10] = -1.0f;
            textRotMat[12] = -1.0f;
            textRotMat[13] = 1.0f;
            textRotMat[15] = 1.0f;
        }

        // Extension Labels
        if (graphNamesOn)
        {
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, 32.0f * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Bold);

            f32 x = ModelXToScreen(-0.5f);
            x = my_Max(x, 0.0f) + 10.0f;

            f32 h = height + 1.0f;

            TraverseLinkedList(Extensions.head, extension_node)
            {
                switch (node->type)
                {
                    case extension_graph:
                        {
                            graph *gph = (graph *)node->extension;

                            if (gph->on && gph->nameOn)
                            {
                                fonsSetColor(FontStash_Context, ThreeFloatColorToU32(gph->colour));
                                fonsDrawText(FontStash_Context, x, h - gph->base, (const char *)gph->name, 0);
                            }
                        }
                        break;
                }
            }
        }

        if (File_Loaded && Contig_Name_Labels->on) // Contig Labels
        {
            glUseProgram(Flat_Shader->shaderProgram);
            glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Contig_Name_Labels->bg);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            u32 ptr = 0;
            vertex vert[4];

            glViewport(0, 0, (s32)width, (s32)height);

            f32 lh = 0.0f;   

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, Contig_Name_Labels->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_CENTER | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);
            fonsSetColor(FontStash_Context, FourFloatColorToU32(Contig_Name_Labels->fg));

            f32 leftPixel = ModelXToScreen(-0.5f);
            f32 totalLength = 0.0f;
            f32 wy0 = ModelYToScreen(0.5f);

            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;
                
                totalLength += (f32)((f64)cont->length / (f64)Number_of_Pixels_1D);

                f32 rightPixel = ModelXToScreen(totalLength - 0.5f);

                if (rightPixel > 0.0f && leftPixel < width)
                {
                    const char *name = (const char *)(Original_Contigs + cont->get_original_contig_id())->name;
                    f32 x = (rightPixel + leftPixel) * 0.5f;
                    f32 y = my_Max(wy0, 0.0f) + 10.0f;
                    f32 textWidth = fonsTextBounds(FontStash_Context, x, y, name, 0, NULL);

                    if (textWidth < (rightPixel - leftPixel))
                    {
                        f32 w2t = 0.5f * textWidth;

                        glUseProgram(Flat_Shader->shaderProgram);

                        vert[0].x = x - w2t;  vert[0].y = y + lh;
                        vert[1].x = x + w2t;  vert[1].y = y + lh;
                        vert[2].x = x + w2t;  vert[2].y = y;
                        vert[3].x = x - w2t;  vert[3].y = y;

                        glBindBuffer(GL_ARRAY_BUFFER, Label_Box_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Label_Box_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUseProgram(UI_Shader->shaderProgram);
                        fonsDrawText(FontStash_Context, x, y, name, 0);
                    }
                }

                leftPixel = rightPixel;
            }

            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textRotMat);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textRotMat);

            f32 topPixel = ModelYToScreen(0.5f);
            f32 wx0 = ModelXToScreen(-0.5f);
            totalLength = 0.0f;

            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;
                
                totalLength += (f32)((f64)cont->length / (f64)Number_of_Pixels_1D);

                f32 bottomPixel = ModelYToScreen(0.5f - totalLength);

                if (topPixel < height && bottomPixel > 0.0f)
                {
                    const char *name = (const char *)(Original_Contigs + cont->get_original_contig_id())->name;
                    f32 y = (topPixel + bottomPixel) * 0.5f;
                    f32 x = my_Max(wx0, 0.0f) + 10.0f;
                    f32 textWidth = fonsTextBounds(FontStash_Context, x, y, name, 0, NULL);

                    if (textWidth < (bottomPixel - topPixel))
                    {
                        f32 tmp = x;
                        x = -y;
                        y = tmp;

                        f32 w2t = 0.5f * textWidth;

                        glUseProgram(Flat_Shader->shaderProgram);

                        vert[0].x = x - w2t;  vert[0].y = y + lh;
                        vert[1].x = x + w2t;  vert[1].y = y + lh;
                        vert[2].x = x + w2t;  vert[2].y = y;
                        vert[3].x = x - w2t;  vert[3].y = y;

                        glBindBuffer(GL_ARRAY_BUFFER, Label_Box_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Label_Box_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUseProgram(UI_Shader->shaderProgram);
                        fonsDrawText(FontStash_Context, x, y, name, 0);
                    }
                }

                topPixel = bottomPixel;
            }

            ChangeSize((s32)width, (s32)height);
        }

        #define MaxTicksPerScaleBar 128
        if (File_Loaded && Scale_Bars->on) // Scale bars
        {
            glUseProgram(Flat_Shader->shaderProgram);
            glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Scale_Bars->bg);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            
            u32 ptr = 0;
            vertex vert[4];

            glViewport(0, 0, (s32)width, (s32)height);

            f32 lh = 0.0f;   

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, Scale_Bars->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_CENTER | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Normal);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);
            fonsSetColor(FontStash_Context, FourFloatColorToU32(Scale_Bars->fg));

            f32 leftPixel = ModelXToScreen(-0.5f);
            f32 rightPixel = ModelXToScreen(0.5f);
            f32 wy0 = ModelYToScreen(0.5);
            f32 offset = 45.0f * Screen_Scale.x;
            f32 y = my_Max(wy0, 0.0f) + offset;
            f32 totalLength = 0.0f;

            f32 bpPerPixel = (f32)((f64)Total_Genome_Length / (f64)(rightPixel - leftPixel));

            GLfloat *bg = (GLfloat *)&Scale_Bars->bg;
                        
            f32 scaleBarWidth = Scale_Bars->size * 4.0f / 20.0f * Screen_Scale.x;
            f32 tickLength = Scale_Bars->size * 3.0f / 20.0f * Screen_Scale.x;

            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;
                
                totalLength += (f32)((f64)cont->length / (f64)Number_of_Pixels_1D);
                rightPixel = ModelXToScreen(totalLength - 0.5f);

                f32 pixelLength = rightPixel - leftPixel;
                f32 startCoord = (f32)((f64)(IsContigInverted(index) ? (cont->startCoord - cont->length) : cont->startCoord) * (f64)Total_Genome_Length / (f64)Number_of_Pixels_1D);

                u32 labelLevels = SubDivideScaleBar(leftPixel, rightPixel, (leftPixel + rightPixel) * 0.5f, bpPerPixel, startCoord);
                u32 labels = 0;
                ForLoop2(labelLevels)
                {
                    labels += (labels + 1);
                }
                labels = my_Min(labels, MaxTicksPerScaleBar);

                if (rightPixel > 0.0f && leftPixel < width)
                {
                    if (labels)
                    {
                        glUseProgram(Flat_Shader->shaderProgram);
                        glUniform4fv(Flat_Shader->colorLocation, 1, bg);

                        vert[0].x = leftPixel + 1.0f;   vert[0].y = y + scaleBarWidth + tickLength + 1.0f + lh;
                        vert[1].x = rightPixel - 1.0f;  vert[1].y = y + scaleBarWidth + tickLength + 1.0f + lh;
                        vert[2].x = rightPixel - 1.0f;  vert[2].y = y;
                        vert[3].x = leftPixel + 1.0f;   vert[3].y = y;

                        glBindBuffer(GL_ARRAY_BUFFER, Scale_Bar_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Scale_Bar_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Scale_Bars->fg);

                        vert[0].x = leftPixel + 1.0f;   vert[0].y = y + scaleBarWidth;
                        vert[1].x = rightPixel - 1.0f;  vert[1].y = y + scaleBarWidth;
                        vert[2].x = rightPixel - 1.0f;  vert[2].y = y;
                        vert[3].x = leftPixel + 1.0f;   vert[3].y = y;

                        glBindBuffer(GL_ARRAY_BUFFER, Scale_Bar_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Scale_Bar_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        f32 fraction = 1.0f / (f32)(labels + 1);
                        f32 distance = 0.0f;
                        ForLoop2(labels)
                        {
                            distance += fraction;
                            f32 x = (pixelLength * distance) + (f32)leftPixel;

                            glUseProgram(Flat_Shader->shaderProgram);

                            vert[0].x = x - (0.5f * scaleBarWidth);  vert[0].y = y + scaleBarWidth + tickLength;
                            vert[1].x = x + (0.5f * scaleBarWidth);  vert[1].y = y + scaleBarWidth + tickLength;
                            vert[2].x = x + (0.5f * scaleBarWidth);  vert[2].y = y + scaleBarWidth;
                            vert[3].x = x - (0.5f * scaleBarWidth);  vert[3].y = y + scaleBarWidth;

                            glBindBuffer(GL_ARRAY_BUFFER, Scale_Bar_Data->vbos[ptr]);
                            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                            glBindVertexArray(Scale_Bar_Data->vaos[ptr++]);
                            glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                            char buff[16];
                            stbsp_snprintf(buff, 16, "%$.2f", (f64)(startCoord + (pixelLength * (IsContigInverted(index) ? (1.0f - distance) : distance) * bpPerPixel)));
                            glUseProgram(UI_Shader->shaderProgram);
                            fonsDrawText(FontStash_Context, x, y + scaleBarWidth + tickLength + 1.0f, buff, 0);
                        }
                    }
                }

                leftPixel = rightPixel;
            }

            ChangeSize((s32)width, (s32)height);
        }
        
        // Finished - add the cross line to the waypoint
        if (File_Loaded && (Waypoint_Edit_Mode || Waypoints_Always_Visible))  // Waypoint Edit Mode  
        {
            u32 ptr = 0;
            vertex vert[4];

            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glViewport(0, 0, (s32)width, (s32)height);
#define DefaultWaypointSize 18.0f
            glUseProgram(Flat_Shader->shaderProgram);
            glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Waypoint_Mode_Data->base);

            // define the vertical line
            f32 lineWidth = Waypoint_Mode_Data->size / DefaultWaypointSize * 0.7f * Screen_Scale.x; 
            f32 lineHeight = Waypoint_Mode_Data->size / DefaultWaypointSize * 8.0f * Screen_Scale.x;

            f32 lh = 0.0f;   

            u32 baseColour = FourFloatColorToU32(Waypoint_Mode_Data->base);
            u32 selectColour = FourFloatColorToU32(Waypoint_Mode_Data->selected);
            
            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, Waypoint_Mode_Data->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);
            fonsSetColor(FontStash_Context, baseColour);

            char buff[4];
            
            // render the waypoints lines
            TraverseLinkedList(Waypoint_Editor->activeWaypoints.next, waypoint) 
            {
                point2f screen = {ModelXToScreen(node->coords.x), ModelYToScreen(-node->coords.y)};
                point2f screenYRange = {ModelYToScreen(0.5f), ModelYToScreen(-0.5f)};
                point2f screenXRange = {ModelXToScreen(-0.5f), ModelXToScreen(0.5f)};

                glUseProgram(Flat_Shader->shaderProgram);
                if (node == Selected_Waypoint)
                {
                    glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Waypoint_Mode_Data->selected);
                }
                
                bool long_horizontal, long_vertical;
                if (Long_Waypoints_Mode == 0) { // vertical line
                    long_horizontal = false;
                    long_vertical = true;
                } else if (Long_Waypoints_Mode == 1) { // horizontal line
                    long_horizontal = true;
                    long_vertical = false;
                } else {  // cross line
                    long_horizontal = true;
                    long_vertical = true;
                }

                // draw the vertical line
                vert[0].x = screen.x - lineWidth;  vert[0].y = long_vertical ? screenYRange.x : screen.y - lineHeight;
                vert[1].x = screen.x - lineWidth;  vert[1].y = long_vertical ? screenYRange.y : screen.y + lineHeight;
                vert[2].x = screen.x + lineWidth;  vert[2].y = long_vertical ? screenYRange.y : screen.y + lineHeight;
                vert[3].x = screen.x + lineWidth;  vert[3].y = long_vertical ? screenYRange.x : screen.y - lineHeight;
                glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // draw the horizontal line (originally: the left part, not sure why Ed draw them seperately, if problem arises we can returen here)  maybe avoid drawing the cross part twice...
                vert[0].x = long_horizontal ? screenXRange.x : screen.x - lineHeight;  vert[0].y = screen.y - lineWidth;
                vert[1].x = long_horizontal ? screenXRange.x : screen.x - lineHeight;  vert[1].y = screen.y + lineWidth;
                vert[2].x = long_horizontal ? screenXRange.y : screen.x + lineHeight;  vert[2].y = screen.y + lineWidth;// vert[2].x = screen.x - lineWidth; 
                vert[3].x = long_horizontal ? screenXRange.y : screen.x + lineHeight;  vert[3].y = screen.y - lineWidth;// vert[3].x = screen.x - lineWidth; 
                glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // draw the horizontal line (originally: the right part, not sure why Ed draw them seperately, if problem arises we can returen here)
                // vert[0].x = screen.x + lineWidth;
                // vert[0].y = screen.y - lineWidth;
                // vert[1].x = screen.x + lineWidth;
                // vert[1].y = screen.y + lineWidth;
                // vert[2].x = screen.x + lineHeight;
                // vert[2].y = screen.y + lineWidth;
                // vert[3].x = screen.x + lineHeight;
                // vert[3].y = screen.y - lineWidth;
                // glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                // glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                // glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                // glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
               
                if (node == Selected_Waypoint)
                {
                    glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Waypoint_Mode_Data->base);
                }

                glUseProgram(UI_Shader->shaderProgram);
                if (node == Selected_Waypoint)
                {
                    fonsSetColor(FontStash_Context, selectColour);
                }
                
                stbsp_snprintf(buff, sizeof(buff), "%d", node->index + 1);
                fonsDrawText(FontStash_Context, screen.x + lineWidth + lineWidth, screen.y - lineWidth - lh, buff, 0);

                if (node == Selected_Waypoint)
                {
                    fonsSetColor(FontStash_Context, baseColour);
                }
            }

            if (Waypoint_Edit_Mode && !UI_On)
            {
                fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
                fonsVertMetrics(FontStash_Context, 0, 0, &lh);
                fonsSetColor(FontStash_Context, FourFloatColorToU32(Waypoint_Mode_Data->text));

                std::vector<char*> helpTexts = {
                    (char*)"Waypoint Edit Mode", 
                    (char*)"W: exit", 
                    (char*)"Left Click: place", 
                    (char*)"Middle Click / Spacebar: delete", 
                    };
                if (Long_Waypoints_Mode == 2) {
                    helpTexts.push_back((char*)"L: both directions");
                } else if (Long_Waypoints_Mode == 1) {
                    helpTexts.push_back((char*)"L: horizontal");
                } else {
                    helpTexts.push_back((char*)"L: vertical");
                }

                f32 textBoxHeight = lh;
                textBoxHeight *= (f32)helpTexts.size();
                textBoxHeight += (f32)helpTexts.size() - 1.0f;
                f32 spacing = 10.0f; // distance from the edge of the text box

                // f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, helpText4, 0, NULL);
                f32 textWidth = 0;
                for (auto i : helpTexts){
                    textWidth = my_Max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, i, 0, NULL)) + 0.5f * spacing;
                }

                glUseProgram(Flat_Shader->shaderProgram);
                glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Waypoint_Mode_Data->bg);

                vert[0].x = width - spacing - textWidth;  vert[0].y = height - spacing - textBoxHeight;
                vert[1].x = width - spacing - textWidth;  vert[1].y = height - spacing;
                vert[2].x = width - spacing;              vert[2].y = height - spacing;
                vert[3].x = width - spacing;              vert[3].y = height - spacing - textBoxHeight;

                glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);

                for (u32 i=0; i< helpTexts.size() ; i++) {
                    fonsDrawText(
                        FontStash_Context, 
                        width - spacing - textWidth, 
                        height - spacing - textBoxHeight + (lh + 1.0f) * i, 
                        helpTexts[i], 
                        0);
                }
            }
        }
       
        // Extension Mode
        if (Extension_Mode && !UI_On)
        {
            u32 ptr = 0;
            vertex vert[4];
            f32 lh = 0.0f;

            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glViewport(0, 0, (s32)width, (s32)height);

#define DefaultExtensionSize 20.0f
            // glUseProgram(Flat_Shader->shaderProgram);
            // glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Waypoint_Mode_Data->base);

            // f32 lineWidth = Waypoint_Mode_Data->size / DefaultWaypointSize * 0.7f * Screen_Scale.x;
            // f32 lineHeight = Waypoint_Mode_Data->size / DefaultWaypointSize * 8.0f * Screen_Scale.x;

            fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);
            fonsSetColor(FontStash_Context, FourFloatColorToU32(Extension_Mode_Data->text));

            f32 textBoxHeight = lh;
            textBoxHeight *= 7.0f;
            textBoxHeight += 6.0f;
            f32 spacing = 10.0f;

            // 6 lines in total
            std::vector<std::string> helpTexts = {
                "Extensions:",
                "X: exit"
            };
            if (Extensions.head)
            {
                TraverseLinkedList(Extensions.head, extension_node)
                {
                    switch (node->type)
                    {
                        case extension_graph:
                            {
                                graph *gph = (graph *)node->extension;

                                if (strstr((char*)gph->name, "coverage"))
                                {
                                    helpTexts.push_back("C: Graph: coverage");
                                }
                                else if (strstr((char*)gph->name, "gap"))
                                {
                                    helpTexts.push_back("G: Graph: gap");
                                }
                                else if (strstr((char*)gph->name, "repeat_density"))
                                {
                                    helpTexts.push_back("R: Graph: repeat_density");
                                }
                                else if (strstr((char*)gph->name, "telomere"))
                                {
                                    helpTexts.push_back("T: Graph: telomere");
                                }
                            }
                            break;
                    }
                }
            }

            f32 textWidth = 0.; 
            for (auto i : helpTexts)
            {
                textWidth = my_Max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, i.c_str(), 0, NULL)) ;
            }
            textWidth += 0.5f * spacing;

            glUseProgram(Flat_Shader->shaderProgram);
            glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Extension_Mode_Data->bg);

            vert[0].x = width - spacing - textWidth; vert[0].y = height - spacing - textBoxHeight;
            vert[1].x = width - spacing - textWidth; vert[1].y = height - spacing;
            vert[2].x = width - spacing;             vert[2].y = height - spacing;
            vert[3].x = width - spacing;             vert[3].y = height - spacing - textBoxHeight;

            glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
            glBindVertexArray(Waypoint_Data->vaos[ptr++]);
            glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

            glUseProgram(UI_Shader->shaderProgram);
            for (u32 i=0; i< helpTexts.size() ; i++) 
            {
                fonsDrawText(
                    FontStash_Context, 
                    width - spacing - textWidth, 
                    height - spacing - textBoxHeight + (lh + 1.0f) * i, 
                    helpTexts[i].c_str(), 
                    0);
            }
        }

        // label to show the selected sequence 从 input sequences 里面选中的片段
        if (Selected_Sequence_Cover_Countor.end_time > 0.)
        {   
            f64 crt_time = GetTime();
            if (crt_time < Selected_Sequence_Cover_Countor.end_time)
            {   
                char buff[128];
                f32 colour[4] = {1.0, 1.0, 1.0, 1.0};
                snprintf(buff, 128, "%s (%u)", (char *)((Original_Contigs+Selected_Sequence_Cover_Countor.original_contig_index)->name), Selected_Sequence_Cover_Countor.idx_within_original_contig+1);

                f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, (char *)buff, 0, NULL);
                ColourGenerator(65, colour);
                // position of text
                f32 textX = ModelXToScreen(
                    (f32)Selected_Sequence_Cover_Countor.map_loc / (f32)Number_of_Pixels_1D -0.5f) 
                    - (0.5f * textWidth);
                f32 textY = ModelYToScreen( 0.5f - (f32)Selected_Sequence_Cover_Countor.map_loc / (f32)Number_of_Pixels_1D);
                
                glUseProgram(UI_Shader->shaderProgram);
                DrawOutlinedText(
                    FontStash_Context, 
                    (nk_colorf *)colour, 
                    (char *)buff, 
                    textX, 
                    textY, 
                    3.0f, true);
                Selected_Sequence_Cover_Countor.plotted = true;
            }
            else
            {
                Selected_Sequence_Cover_Countor.clear();
                Redisplay = 1;
            }
        }

        // Scaff Bars
        if (File_Loaded && (Scaff_Edit_Mode || Scaffs_Always_Visible))
        {
            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glViewport(0, 0, (s32)width, (s32)height);

            u32 ptr = 0;
            vertex vert[4];
            f32 barColour[4] = {1.0f, 1.0f, 1.0f, 0.5f};

            f32 lh = 0.0f;   
            fonsClearState(FontStash_Context);
#define DefaultScaffSize 40.0f
            fonsSetSize(FontStash_Context, Scaff_Mode_Data->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_MIDDLE | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);

            char buff[128];
            f32 position = 0.0f;
            f32 start = 0.0f;
            u32 scaffId = Contigs->contigs_arr->scaffId;
            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;

                if (cont->scaffId != scaffId)
                {
                    if (scaffId)
                    {
                        vert[0].x = ModelXToScreen(start - 0.5f);     vert[0].y = ModelYToScreen(0.5f - start);
                        vert[1].x = ModelXToScreen(start - 0.5f);     vert[1].y = ModelYToScreen(0.5f - position);
                        vert[2].x = ModelXToScreen(position - 0.5f);  vert[2].y = ModelYToScreen(0.5f - position);
                        vert[3].x = ModelXToScreen(position - 0.5f);  vert[3].y = ModelYToScreen(0.5f - start);

                        ColourGenerator((u32)scaffId, (f32 *)barColour);
                        u32 colour = ThreeFloatColorToU32(*((nk_colorf *)barColour));

                        glUseProgram(Flat_Shader->shaderProgram);
                        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&barColour);

                        glBindBuffer(GL_ARRAY_BUFFER, Scaff_Bar_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Scaff_Bar_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUseProgram(UI_Shader->shaderProgram);
                        fonsSetColor(FontStash_Context, colour);

                        stbsp_snprintf(buff, sizeof(buff), "Scaffold %u", scaffId);
                        f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, buff, 0, NULL);
                        fonsDrawText(FontStash_Context, ModelXToScreen(0.5f * (position + start - 1.0f)) - (0.5f * textWidth), ModelYToScreen(0.5f - start) - lh, buff, 0);
                    }

                    start = position;
                    scaffId = cont->scaffId;
                }

                position += ((f32)cont->length / (f32)Number_of_Pixels_1D);
            }

            if (scaffId)
            {
                vert[0].x = ModelXToScreen(start - 0.5f);     vert[0].y = ModelYToScreen(0.5f - start);
                vert[1].x = ModelXToScreen(start - 0.5f);     vert[1].y = ModelYToScreen(0.5f - position);
                vert[2].x = ModelXToScreen(position - 0.5f);  vert[2].y = ModelYToScreen(0.5f - position);
                vert[3].x = ModelXToScreen(position - 0.5f);  vert[3].y = ModelYToScreen(0.5f - start);

                ColourGenerator((u32)scaffId, (f32 *)barColour);
                u32 colour = FourFloatColorToU32(*((nk_colorf *)barColour));

                glUseProgram(Flat_Shader->shaderProgram);
                glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&barColour);

                glBindBuffer(GL_ARRAY_BUFFER, Scaff_Bar_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Scaff_Bar_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);
                fonsSetColor(FontStash_Context, colour);

                stbsp_snprintf(buff, sizeof(buff), "Scaffold %u", scaffId);
                f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, buff, 0, NULL);
                fonsDrawText(FontStash_Context, ModelXToScreen(0.5f * (position + start - 1.0f)) - (0.5f * textWidth), ModelYToScreen(0.5f - start) - lh, buff, 0);
            }

            if (Scaff_Edit_Mode && !UI_On)
            {
                fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
                fonsVertMetrics(FontStash_Context, 0, 0, &lh);
                fonsSetColor(FontStash_Context, FourFloatColorToU32(Scaff_Mode_Data->text));

                f32 textBoxHeight = lh;
                textBoxHeight *= 7.0f;
                textBoxHeight += 6.0f;
                f32 spacing = 10.0f;

                std::vector<std::string> helpTexts = {
                    "Scaffold Edit Mode", 
                    "S: exit", 
                    "Left Click: place", 
                    "Middle Click / Spacebar: delete", 
                    "Shift-D: delete all", 
                    "A (Hold): flood fill", 
                    "Shift-A (Hold): flood fill and override"
                };

                f32 textWidth =0.f;
                for (const auto& tmp:helpTexts) 
                    textWidth = std::max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, tmp.c_str(), 0, NULL));

                glUseProgram(Flat_Shader->shaderProgram);
                glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Scaff_Mode_Data->bg);

                vert[0].x = width - spacing - textWidth;  vert[0].y = height - spacing - textBoxHeight;
                vert[1].x = width - spacing - textWidth;  vert[1].y = height - spacing;
                vert[2].x = width - spacing;              vert[2].y = height - spacing;
                vert[3].x = width - spacing;              vert[3].y = height - spacing - textBoxHeight;

                glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);
                for (u32 i=0; i< helpTexts.size() ; i++) 
                {
                    fonsDrawText(
                        FontStash_Context, 
                        width - spacing - textWidth, 
                        height - spacing - textBoxHeight + (lh + 1.0f) * i, 
                        helpTexts[i].c_str(), 
                        0);
                }
            }
        } // scaff_mode

        // draw the screen for select fragments for sorting
        if (File_Loaded && !UI_On && Select_Sort_Area_Mode)
        {   
            f32 lh = 0.f;
            f32 start_fraction = (f32)auto_curation_state.get_start()/(f32)Number_of_Pixels_1D;
            f32 end_fraction   = (f32)auto_curation_state.get_end()  /(f32)Number_of_Pixels_1D;

            // selected frags into a string
            SelectArea select_area;
            auto_curation_state.get_selected_fragments(select_area, Map_State, Number_of_Pixels_1D, Contigs);

            { // draw the help text in the bottom right corner
                fonsSetFont(FontStash_Context, Font_Bold);
                fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
                fonsVertMetrics(FontStash_Context, 0, 0, &lh);
                fonsSetColor(FontStash_Context, FourFloatColorToU32(Scaff_Mode_Data->text));

                std::vector<std::string> helpTexts = {
                    "Select/Exclude area for sorting",
                    "F: exit",
                    "S: clear select area", 
                    "C/Z: cut / cancel cut",
                    "Q/W: quit / redo edit",
                    "Space: Pixel sort",
                    "Left Click: un/select the fragment",
                    fmt::format("Up/Down: inc/dec Cut threshold: {:.4f}", auto_curation_state.auto_cut_threshold).c_str(),
                    fmt::format("Left/Right: change Sort Mode ({})", auto_curation_state.sort_mode_names[auto_curation_state.sort_mode]),
                    // fmt::format("Selected fragments number: {}", select_area.selected_frag_ids.size()),
                    // fmt::format("Select pixel range: {} - {}", auto_curation_state.get_start(), auto_curation_state.get_end())
                };

                f32 textBoxHeight = (f32)helpTexts.size() * (lh + 1.0f) - 1.0f;
                f32 spacing = 10.0f;

                f32 textWidth = 0.f; 
                for (const auto& i :helpTexts){
                    textWidth = my_Max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, i.c_str(), 0, NULL));
                } 
                textWidth = my_Min(textWidth, width - 2 * spacing);

                glUseProgram(Flat_Shader->shaderProgram);
                glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Scaff_Mode_Data->bg);
                point2f vert[4];
                vert[0].x = width - spacing - textWidth; vert[0].y = height - spacing - textBoxHeight;
                vert[1].x = width - spacing - textWidth; vert[1].y = height - spacing;
                vert[2].x = width - spacing;             vert[2].y = height - spacing;
                vert[3].x = width - spacing;             vert[3].y = height - spacing - textBoxHeight;
                
                u32 ptr = 0;
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);

                for (int i = 0; i < helpTexts.size(); i++)
                {
                    fonsDrawText(
                        FontStash_Context, 
                        width - spacing - textWidth, 
                        height - spacing - textBoxHeight + (i * (lh + 1.0f)), 
                        helpTexts[i].c_str(), 
                        0);
                }
            }

            { // paint the selected area
                if (start_fraction >= 0 && end_fraction >= 0 )
                {   
                    u32 ptr = 0;
                    point2f vert[4];

                    // draw the start & end point
                    {   
                        f32 line_width = 0.002f / Camera_Position.z;
                        f32 mask_color[4] = {1.0f, 0.f, 0.f, 0.8f}; // red
                        glUseProgram(Flat_Shader->shaderProgram);
                        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&mask_color);
                        for (auto loc_fraction : {start_fraction, end_fraction})
                        {   
                            vert[0].x = ModelXToScreen(loc_fraction - 0.5f - line_width);  vert[0].y = ModelYToScreen(0.5f - loc_fraction + line_width);
                            vert[1].x = ModelXToScreen(loc_fraction - 0.5f - line_width);  vert[1].y = ModelYToScreen(0.5f - loc_fraction - line_width);
                            vert[2].x = ModelXToScreen(loc_fraction - 0.5f + line_width);  vert[2].y = ModelYToScreen(0.5f - loc_fraction - line_width);
                            vert[3].x = ModelXToScreen(loc_fraction - 0.5f + line_width);  vert[3].y = ModelYToScreen(0.5f - loc_fraction + line_width);

                            glBindBuffer(GL_ARRAY_BUFFER, Scaff_Bar_Data->vbos[ptr]);
                            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                            glBindVertexArray(Scaff_Bar_Data->vaos[ptr++]);
                            glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
                        }
                    }

                    // draw the cover on selected area
                    {   
                        // f32 mask_color[4] = {0.906f, 0.03921f, 0.949f, 0.5f}; 
                        vert[0].x = ModelXToScreen(start_fraction - 0.5f);     vert[0].y = ModelYToScreen(0.5f - start_fraction);
                        vert[1].x = ModelXToScreen(start_fraction - 0.5f);     vert[1].y = ModelYToScreen(0.5f - end_fraction);
                        vert[2].x = ModelXToScreen(end_fraction - 0.5f);       vert[2].y = ModelYToScreen(0.5f - end_fraction);
                        vert[3].x = ModelXToScreen(end_fraction - 0.5f);       vert[3].y = ModelYToScreen(0.5f - start_fraction);

                        f32 font_color[4] = {0.f, 0.f, 0.f, 1.f};
                        u32 colour = FourFloatColorToU32(*((nk_colorf *)font_color));

                        glUseProgram(Flat_Shader->shaderProgram);
                        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&auto_curation_state.mask_color);

                        glBindBuffer(GL_ARRAY_BUFFER, Scaff_Bar_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Scaff_Bar_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUseProgram(UI_Shader->shaderProgram);
                        fonsSetColor(FontStash_Context, colour);
                        
                        std::string buff = fmt::format(
                            "{}: ({}) pixels, ({}) fragments", 
                            auto_curation_state.selected_or_exclude==0?"Selected" :"Excluded", 
                            std::abs(auto_curation_state.get_end() - auto_curation_state.get_start()), 
                            select_area.selected_frag_ids.size());

                        f32 lh = 0.0f;
                        f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, buff.c_str(), 0, NULL);
                        fonsDrawText(
                            FontStash_Context, 
                            ModelXToScreen( 0.5f * (end_fraction + start_fraction ) - 0.5f) - (0.5f * textWidth), 
                            ModelYToScreen(0.5f - 0.5f*(start_fraction + end_fraction)) - lh * 0.5f, buff.c_str(), 0);
                    }
                }
            }
        } // select_sort_area

        // Meta Tags
        if (File_Loaded && (MetaData_Edit_Mode || MetaData_Always_Visible))
        {
            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glViewport(0, 0, (s32)width, (s32)height);

            f32 colour[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            vertex vert[4];
            u32 ptr = 0;
            f32 barColour[4] = {1.0f, 1.0f, 1.0f, 0.5f};

            f32 lh = 0.0f;   
            fonsClearState(FontStash_Context);
#define DefaultMetaDataSize 20.0f
            fonsSetSize(FontStash_Context, MetaData_Mode_Data->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_MIDDLE | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);

            f32 end_contig = 0.0f, start_contig = 0.0f;
            u32 scaffId = Contigs->contigs_arr->scaffId;
            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;
                end_contig += ((f32)cont->length / (f32)Number_of_Pixels_1D); // end of the contig
                if (*cont->metaDataFlags)
                {
                    u32 tmp = 0; // used to count the number of tags drawn
                    bool haplotigTagged = false;
                    ForLoop2(ArrayCount(Meta_Data->tags))
                    {
                        if (*cont->metaDataFlags & ((u64)1 << index2))
                        {
                            f32 textWidth = fonsTextBounds(FontStash_Context, 0, 0, (char *)Meta_Data->tags[index2], 0, NULL);
                            ColourGenerator(index2 + 1, colour);
                            // fonsSetColor(FontStash_Context, FourFloatColorToU32(*((nk_colorf *)colour)));
                            // fonsDrawText(FontStash_Context, ModelXToScreen(0.5f * (end_contig + start_contig - 1.0f)) - (0.5f * textWidth), ModelYToScreen((0.5f * (1.0f - end_contig - start_contig))) - (lh * (f32)(++tmp)), (char *)Meta_Data->tags[index2], 0);
                            if (meta_outline->on)
                            {
                                // position of text
                                f32 textX = ModelXToScreen(0.5f * (end_contig + start_contig - 1.0f)) - (0.5f * textWidth);
                                f32 textY = ModelYToScreen((0.5f * (1.0f - end_contig - start_contig))) - (lh * (f32)(++tmp));

                                DrawOutlinedText(FontStash_Context, (nk_colorf *)colour, (char *)Meta_Data->tags[index2], textX, textY);
                            }
                            else
                            {
                                fonsSetColor(FontStash_Context, FourFloatColorToU32(*((nk_colorf *)colour)));
                                fonsDrawText(
                                    FontStash_Context,
                                    ModelXToScreen(0.5f * (end_contig + start_contig - 1.0f)) - (0.5f * textWidth), 
                                    ModelYToScreen((0.5f * (1.0f - end_contig - start_contig))) - (lh * (f32)(++tmp)), 
                                    (char *)Meta_Data->tags[index2], 
                                    0);
                            }

                            // Check if the tag is "Haplotig"
                            if (strcmp((char *)Meta_Data->tags[index2], "Haplotig") == 0)
                            {
                                haplotigTagged = true;
                            }
                        }
                    }

                    // draw the grey out mask
                    std::string grey_out_tag = Grey_Out_Settings->is_grey_out(cont->metaDataFlags, Meta_Data);
                    if (!grey_out_tag.empty())
                    {
                        vert[0].x = ModelXToScreen(start_contig - 0.5f);     vert[0].y = ModelYToScreen(0.5f - start_contig);
                        vert[1].x = ModelXToScreen(start_contig - 0.5f);     vert[1].y = ModelYToScreen(0.5f - end_contig);
                        vert[2].x = ModelXToScreen(end_contig - 0.5f);       vert[2].y = ModelYToScreen(0.5f - end_contig);
                        vert[3].x = ModelXToScreen(end_contig - 0.5f);       vert[3].y = ModelYToScreen(0.5f - start_contig);

                        ColourGenerator((u32)scaffId, (f32 *)barColour);
                        u32 colour = FourFloatColorToU32(*((nk_colorf *)barColour));

                        glUseProgram(Flat_Shader->shaderProgram);
                        glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&barColour);

                        glBindBuffer(GL_ARRAY_BUFFER, Scaff_Bar_Data->vbos[ptr]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                        glBindVertexArray(Scaff_Bar_Data->vaos[ptr++]);
                        glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                        glUseProgram(UI_Shader->shaderProgram);
                        fonsSetColor(FontStash_Context, colour);
                    }
                }
                start_contig = end_contig;
                scaffId = cont->scaffId;
            }

            if (MetaData_Edit_Mode && !UI_On)
            {
                u32 ptr = 0;
                vertex vert[4];

                fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
                fonsVertMetrics(FontStash_Context, 0, 0, &lh);
                fonsSetColor(FontStash_Context, FourFloatColorToU32(MetaData_Mode_Data->text));

                f32 textBoxHeight = lh;
                textBoxHeight *= 7.0f;
                textBoxHeight += 6.0f;
                f32 spacing = 10.0f;

                char helpText7[128];
                const char *activeTag = (const char *)Meta_Data->tags[MetaData_Active_Tag];
                stbsp_snprintf(helpText7, sizeof(helpText7), "Active Tag: %s", strlen(activeTag) ? activeTag : "<NA>");

                std::vector<std::string> helpTexts = {
                    "MetaData Tag Mode", 
                    "M: exit", 
                    "Left Click: place", 
                    "Middle Click / Spacebar: delete", 
                    "Shift-D: delete all", 
                    "Arrow Keys: select active tag", 
                    helpText7
                };

                f32 textWidth = 0.f;
                for (const auto& tmp:helpTexts) 
                    textWidth = std::max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, tmp.c_str(), 0, NULL));

                glUseProgram(Flat_Shader->shaderProgram);
                glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&MetaData_Mode_Data->bg);

                vert[0].x = width - spacing - textWidth;  vert[0].y = height - spacing - textBoxHeight;
                vert[1].x = width - spacing - textWidth;  vert[1].y = height - spacing;
                vert[2].x = width - spacing;              vert[2].y = height - spacing;
                vert[3].x = width - spacing;              vert[3].y = height - spacing - textBoxHeight;

                glBindBuffer(GL_ARRAY_BUFFER, Waypoint_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Waypoint_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);
                for (u32 i=0; i< helpTexts.size() ; i++) {
                    fonsDrawText(
                        FontStash_Context, 
                        width - spacing - textWidth, 
                        height - spacing - textBoxHeight + (lh + 1.0f) * i, 
                        helpTexts[i].c_str(), 
                        0);
                }
            }
        }
        
        // Tool Tip
        if (File_Loaded && Tool_Tip->on && !Edit_Mode && !UI_On)
        {
            glUseProgram(Flat_Shader->shaderProgram);
            glUniform4fv(Flat_Shader->colorLocation, 1, (GLfloat *)&Tool_Tip->bg);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            vertex vert[4];

            glViewport(0, 0, (s32)width, (s32)height);

            f32 lh = 0.0f;   

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, Tool_Tip->size * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
            fonsSetFont(FontStash_Context, Font_Normal);
            fonsVertMetrics(FontStash_Context, 0, 0, &lh);
            fonsSetColor(FontStash_Context, FourFloatColorToU32(Tool_Tip->fg));
           
            // Extension info, extra lines
            u32 nExtra = 0;
            f32 longestExtraLineLength = 0.0f;
            {
                if (Extensions.head)
                {
                    char buff[128];
                    TraverseLinkedList(Extensions.head, extension_node)
                    {
                        switch (node->type)
                        {
                            case extension_graph:
                                {
                                    graph *gph = (graph *)node->extension;
                                    if (gph->on)
                                    {
                                        glBindBuffer(GL_TEXTURE_BUFFER, Contact_Matrix->pixelRearrangmentLookupBuffer);
                                        u32 *buffer = (u32 *)glMapBufferRange(GL_TEXTURE_BUFFER, Tool_Tip_Move.pixels.x * sizeof(u32), sizeof(u32), GL_MAP_READ_BIT);

                                        stbsp_snprintf(buff, sizeof(buff), "%s: %$d", (char *)gph->name, gph->data[*buffer]);
                                        ++nExtra;
                                        longestExtraLineLength = my_Max(longestExtraLineLength, fonsTextBounds(FontStash_Context, 0, 0, buff, 0, NULL));

                                        glUnmapBuffer(GL_TEXTURE_BUFFER);
                                        glBindBuffer(GL_TEXTURE_BUFFER, 0);
                                    }
                                }
                                break;
                        }
                    }
                }
            }

            f32 textBoxHeight = lh;
            textBoxHeight *= (3.0f + (f32)nExtra);
            textBoxHeight += (2.0f + (f32)nExtra);

            u32 id1 = Map_State->get_original_contig_id(Tool_Tip_Move.pixels.x);
            u32 id2 = Map_State->get_original_contig_id(Tool_Tip_Move.pixels.y);
            u32 coord1 = Map_State->contigRelCoords[Tool_Tip_Move.pixels.x];
            u32 coord2 = Map_State->contigRelCoords[Tool_Tip_Move.pixels.y];
            
            f64 bpPerPixel = (f64)Total_Genome_Length / (f64)Number_of_Pixels_1D;

            char line1[64];
            char *line2 = (char *)"vs";
            char line3[64];

            auto NicePrint = [bpPerPixel](u32 id, u32 coord, char *buffer)
            {
                f64 pos = (f64)coord * bpPerPixel;
                stbsp_snprintf(buffer, 64, "%s %'u %sbp", (Original_Contigs + id)->name, (u32)(pos / (pos > 1000.0 ? 1000.0 : 1.0)), pos > 1000.0 ? "K" : "");
            };

            NicePrint(id1, coord1, line1);
            NicePrint(id2, coord2, line3);

            f32 textWidth_1 = fonsTextBounds(FontStash_Context, 0, 0, line1, 0, NULL);
            f32 textWidth_2 = fonsTextBounds(FontStash_Context, 0, 0, line2, 0, NULL);
            f32 textWidth_3 = fonsTextBounds(FontStash_Context, 0, 0, line3, 0, NULL);
            f32 textWidth = my_Max(textWidth_1, textWidth_2);
            textWidth = my_Max(textWidth, textWidth_3);
            textWidth = my_Max(textWidth, longestExtraLineLength);

            f32 spacing = 12.0f;

            glUseProgram(Flat_Shader->shaderProgram);

            vert[0].x = ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing;             vert[0].y = ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing;
            vert[1].x = ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing;             vert[1].y = ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing + textBoxHeight;
            vert[2].x = ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing + textWidth; vert[2].y = ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing + textBoxHeight;
            vert[3].x = ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing + textWidth; vert[3].y = ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing;

            glBindBuffer(GL_ARRAY_BUFFER, Tool_Tip_Data->vbos[0]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
            glBindVertexArray(Tool_Tip_Data->vaos[0]);
            glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

            glUseProgram(UI_Shader->shaderProgram);
            fonsDrawText(FontStash_Context, ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing, 
                    ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing, line1, 0);
            fonsDrawText(FontStash_Context, ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing, 
                    ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing + lh + 1.0f, line2, 0);
            fonsDrawText(FontStash_Context, ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing, 
                    ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing + (2.0f * lh) + 2.0f, line3, 0);

            {
                if (Extensions.head)
                {
                    u32 count = 0;
                    char buff[128];
                    TraverseLinkedList(Extensions.head, extension_node)
                    {
                        switch (node->type)
                        {
                            case extension_graph:
                                {
                                    graph *gph = (graph *)node->extension;
                                    if (gph->on)
                                    {
                                        glBindBuffer(GL_TEXTURE_BUFFER, Contact_Matrix->pixelRearrangmentLookupBuffer);
                                        u32 *buffer = (u32 *)glMapBufferRange(GL_TEXTURE_BUFFER, Tool_Tip_Move.pixels.x * sizeof(u32), sizeof(u32), GL_MAP_READ_BIT);

                                        stbsp_snprintf(buff, sizeof(buff), "%s: %$d", (char *)gph->name, gph->data[*buffer]);

                                        fonsDrawText(FontStash_Context, ModelXToScreen(Tool_Tip_Move.worldCoords.x) + spacing, 
                                                ModelYToScreen(-Tool_Tip_Move.worldCoords.y) + spacing + ((2.0f + (f32)(++count)) * (lh + 1.0f)), buff, 0);

                                        glUnmapBuffer(GL_TEXTURE_BUFFER);
                                        glBindBuffer(GL_TEXTURE_BUFFER, 0);
                                    }
                                }
                                break;
                        }
                    }
                }
            }
        }

        // Edit Mode
        if (File_Loaded && Edit_Mode && !UI_On) // Edit Mode
        {
            u32 ptr = 0;
            vertex vert[4];

            glUseProgram(Flat_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);

            glUseProgram(Flat_Shader->shaderProgram);
            glViewport(0, 0, (s32)width, (s32)height);

            f32 color[4];
            f32* source;
            if (Edit_Pixels.editing) // edit color 
            {
                if (Global_Edit_Invert_Flag) source = (f32*)&Edit_Mode_Colours->invSelect;
                else source = (f32*)&Edit_Mode_Colours->select;
            } 
            else source = (f32*)&Edit_Mode_Colours->preSelect; // pre-edit color
            ForLoop(4)
            {
                color[index] = source[index];
            }
            f32 alpha = color[3];
            color[3] = 1.0f;

            glUniform4fv(Flat_Shader->colorLocation, 1, color);

            f32 lineWidth = 0.005f / Camera_Position.z;
            
            { // draw the two squared dots at the beginning and end of the selected fragment
                vert[0].x = ModelXToScreen(Edit_Pixels.worldCoords.x - lineWidth);
                vert[0].y = ModelYToScreen(lineWidth - Edit_Pixels.worldCoords.x);
                vert[1].x = ModelXToScreen(Edit_Pixels.worldCoords.x - lineWidth);
                vert[1].y = ModelYToScreen(-lineWidth - Edit_Pixels.worldCoords.x);
                vert[2].x = ModelXToScreen(Edit_Pixels.worldCoords.x + lineWidth);
                vert[2].y = ModelYToScreen(-lineWidth - Edit_Pixels.worldCoords.x);
                vert[3].x = ModelXToScreen(Edit_Pixels.worldCoords.x + lineWidth);
                vert[3].y = ModelYToScreen(lineWidth - Edit_Pixels.worldCoords.x);

                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                vert[0].x = ModelXToScreen(Edit_Pixels.worldCoords.y - lineWidth);
                vert[0].y = ModelYToScreen(lineWidth - Edit_Pixels.worldCoords.y);
                vert[1].x = ModelXToScreen(Edit_Pixels.worldCoords.y - lineWidth);
                vert[1].y = ModelYToScreen(-lineWidth - Edit_Pixels.worldCoords.y);
                vert[2].x = ModelXToScreen(Edit_Pixels.worldCoords.y + lineWidth);
                vert[2].y = ModelYToScreen(-lineWidth - Edit_Pixels.worldCoords.y);
                vert[3].x = ModelXToScreen(Edit_Pixels.worldCoords.y + lineWidth);
                vert[3].y = ModelYToScreen(lineWidth - Edit_Pixels.worldCoords.y);

                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }
            
            f32 min = my_Min(Edit_Pixels.worldCoords.x, Edit_Pixels.worldCoords.y);
            f32 max = my_Max(Edit_Pixels.worldCoords.x, Edit_Pixels.worldCoords.y);

            if (Global_Edit_Invert_Flag) // draw the two arrows
            {
                f32 spacing = 0.002f / Camera_Position.z;
                f32 arrowWidth = 0.01f / Camera_Position.z;
                f32 arrowHeight = arrowWidth * 0.65f;
                f32 recHeight = arrowHeight * 0.65f;

                // draw the the triangle part 
                vert[0].x = ModelXToScreen(min + spacing);              vert[0].y = ModelYToScreen(arrowHeight + spacing - min);
                vert[1].x = ModelXToScreen(min + spacing + arrowWidth); vert[1].y = ModelYToScreen(spacing - min);
                vert[2].x = ModelXToScreen(min + spacing + arrowWidth); vert[2].y = ModelYToScreen(arrowHeight + spacing - min);
                vert[3].x = ModelXToScreen(min + spacing + arrowWidth); vert[3].y = ModelYToScreen((2.0f * arrowHeight) + spacing - min);
                
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // draw the rectangle part
                vert[0].x = ModelXToScreen(min + spacing + arrowWidth); vert[0].y = ModelYToScreen((arrowHeight + 0.5f * recHeight) + spacing - min);
                vert[1].x = ModelXToScreen(min + spacing + arrowWidth); vert[1].y = ModelYToScreen((arrowHeight - 0.5f * recHeight) + spacing - min);
                vert[2].x = ModelXToScreen(max - spacing);              vert[2].y = ModelYToScreen((arrowHeight - 0.5f * recHeight) + spacing - min);
                vert[3].x = ModelXToScreen(max - spacing);              vert[3].y = ModelYToScreen((arrowHeight + 0.5f * recHeight) + spacing - min);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }

            { // draw the interacted area
                
                color[3] = alpha;
                glUniform4fv(Flat_Shader->colorLocation, 1, color);

                // upper vertical part
                vert[0].x = ModelXToScreen(min); vert[0].y = ModelYToScreen(0.5f);
                vert[1].x = ModelXToScreen(min); vert[1].y = ModelYToScreen(-min);
                vert[2].x = ModelXToScreen(max); vert[2].y = ModelYToScreen(-min);
                vert[3].x = ModelXToScreen(max); vert[3].y = ModelYToScreen(0.5f);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // left horizontal part
                vert[0].x = ModelXToScreen(-0.5f); vert[0].y = ModelYToScreen(-min);
                vert[1].x = ModelXToScreen(-0.5f); vert[1].y = ModelYToScreen(-max);
                vert[2].x = ModelXToScreen(min);   vert[2].y = ModelYToScreen(-max);
                vert[3].x = ModelXToScreen(min);   vert[3].y = ModelYToScreen(-min);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // lower vertical part
                vert[0].x = ModelXToScreen(min); vert[0].y = ModelYToScreen(-max);
                vert[1].x = ModelXToScreen(min); vert[1].y = ModelYToScreen(-0.5f);
                vert[2].x = ModelXToScreen(max); vert[2].y = ModelYToScreen(-0.5f);
                vert[3].x = ModelXToScreen(max); vert[3].y = ModelYToScreen(-max);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // right horizontal part
                vert[0].x = ModelXToScreen(max);  vert[0].y = ModelYToScreen(-min);
                vert[1].x = ModelXToScreen(max);  vert[1].y = ModelYToScreen(-max);
                vert[2].x = ModelXToScreen(0.5f); vert[2].y = ModelYToScreen(-max);
                vert[3].x = ModelXToScreen(0.5f); vert[3].y = ModelYToScreen(-min);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                // ceter part
                vert[0].x = ModelXToScreen(min); vert[0].y = ModelYToScreen(-min);
                vert[1].x = ModelXToScreen(min); vert[1].y = ModelYToScreen(-max);
                vert[2].x = ModelXToScreen(max); vert[2].y = ModelYToScreen(-max);
                vert[3].x = ModelXToScreen(max); vert[3].y = ModelYToScreen(-min);
                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
            }

            { // draw the text by the selected (pre-select) fragment
                f32 lh = 0.0f;   
                
                fonsClearState(FontStash_Context);
                fonsSetSize(FontStash_Context, 18.0f * Screen_Scale.x);
                fonsSetAlign(FontStash_Context, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
                fonsSetFont(FontStash_Context, Font_Normal);
                fonsVertMetrics(FontStash_Context, 0, 0, &lh);
                fonsSetColor(FontStash_Context, FourFloatColorToU32(Edit_Mode_Colours->fg));
                
                f32 textBoxHeight = lh;
                textBoxHeight *= Edit_Pixels.editing ? 3.0f : 1.0f;
                textBoxHeight += Edit_Pixels.editing ? 3.0f : 0.0f;

                static char line1[64];
                static u32 line1Done = 0;
                char line2[64];

                char *midLineNoInv = (char *)"moved to";
                char *midLineInv = (char *)"inverted and moved to";
                char *midLine = Global_Edit_Invert_Flag ? midLineInv : midLineNoInv;

                u32 pix1 = my_Min(Edit_Pixels.pixels.x, Edit_Pixels.pixels.y);
                u32 pix2 = my_Max(Edit_Pixels.pixels.x, Edit_Pixels.pixels.y);

                if (Edit_Pixels.editing && line1Done)
                {
                    pix1 = pix1 ? pix1 - 1 : (pix2 < (Number_of_Pixels_1D - 1) ? pix2 + 1 : pix2);
                }

                original_contig *cont = Original_Contigs + Map_State->get_original_contig_id(pix1);

                u32 nPixels = Number_of_Pixels_1D;
                f64 bpPerPixel = (f64)Total_Genome_Length / (f64)nPixels;

                f64 bpStart = bpPerPixel * (f64)Map_State->contigRelCoords[pix1];
                
                if (Edit_Pixels.editing)
                {
                    stbsp_snprintf(line2, 64, "%s[%$.2fbp]", cont->name, bpStart);
                }
                else if (line1Done)
                {
                    line1Done = 0;
                }
                
                if (!line1Done)
                {
                    f64 bpEnd = bpPerPixel * (f64)Map_State->contigRelCoords[pix2];
                    original_contig *cont2 = Original_Contigs + Map_State->get_original_contig_id(pix2);
                    stbsp_snprintf(line1, 64, "%s[%$.2fbp] to %s[%$.2fbp]", cont->name, bpStart, cont2->name, bpEnd);
                    if (Edit_Pixels.editing)
                    {
                        line1Done = 1;
                    }
                }

                f32 textWidth_1 = fonsTextBounds(FontStash_Context, 0, 0, line1, 0, NULL);
                f32 textWidth_2 = fonsTextBounds(FontStash_Context, 0, 0, line2, 0, NULL);
                f32 textWidth_3 = fonsTextBounds(FontStash_Context, 0, 0, midLine, 0, NULL);
                f32 textWidth = my_Max(textWidth_1, textWidth_2);
                textWidth = my_Max(textWidth, textWidth_3);

                f32 spacing = 3.0f;

                glUniform4fv(Flat_Shader->colorLocation, 1, (f32 *)&Edit_Mode_Colours->bg);

                vert[0].x = ModelXToScreen(min) - spacing - textWidth;
                vert[0].y = ModelYToScreen(-max) + spacing;
                vert[1].x = ModelXToScreen(min) - spacing - textWidth;
                vert[1].y = ModelYToScreen(-max) + spacing + textBoxHeight;
                vert[2].x = ModelXToScreen(min) - spacing;
                vert[2].y = ModelYToScreen(-max) + spacing + textBoxHeight;
                vert[3].x = ModelXToScreen(min) - spacing;
                vert[3].y = ModelYToScreen(-max) + spacing;

                glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                glUseProgram(UI_Shader->shaderProgram);
                fonsDrawText(FontStash_Context, ModelXToScreen(min) - spacing - textWidth, ModelYToScreen(-max) + spacing, line1, 0);

                if (Edit_Pixels.editing)
                {
                    fonsDrawText(FontStash_Context, ModelXToScreen(min) - spacing - textWidth, ModelYToScreen(-max) + spacing + lh + 1.0f, midLine, 0);
                    fonsDrawText(FontStash_Context, ModelXToScreen(min) - spacing - textWidth, ModelYToScreen(-max) + spacing + (2.0f * (lh + 1.0f)), line2, 0);
                }

                if (Edit_Pixels.snap)
                {
                    char *text = (char *)"Snap Mode On";
                    textWidth = fonsTextBounds(FontStash_Context, 0, 0, text, 0, NULL);
                    glUseProgram(Flat_Shader->shaderProgram);
                    vert[0].x = ModelXToScreen(min) - spacing - textWidth; vert[0].y = ModelYToScreen(-min) + spacing;
                    vert[1].x = ModelXToScreen(min) - spacing - textWidth; vert[1].y = ModelYToScreen(-min) + spacing + lh;
                    vert[2].x = ModelXToScreen(min) - spacing;             vert[2].y = ModelYToScreen(-min) + spacing + lh;
                    vert[3].x = ModelXToScreen(min) - spacing;             vert[3].y = ModelYToScreen(-min) + spacing;

                    glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                    glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                    glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);
                    
                    glUseProgram(UI_Shader->shaderProgram);
                    fonsDrawText(FontStash_Context, ModelXToScreen(min) - spacing - textWidth, ModelYToScreen(-min) + spacing, text, 0);
                }

                { // draw the help text in the bottom right corner
                    fonsSetFont(FontStash_Context, Font_Bold);
                    fonsSetSize(FontStash_Context, 24.0f * Screen_Scale.x);
                    fonsVertMetrics(FontStash_Context, 0, 0, &lh);

                    std::vector<char*> helpTexts = {
                        (char *)"Edit Mode",
                        (char *)"E: exit, Q: undo, W: redo",
                        (char *)"Left Click: pickup, place",
                        (char *)"S: toggle snap mode",
                        (char *)"Middle Click / Spacebar: pickup whole sequence or (hold Shift): scaffold",
                        (char *)"Middle Click / Spacebar (while editing): invert sequence"
                    };

                    textBoxHeight = (f32)helpTexts.size() * (lh + 1.0f) - 1.0f;
                    spacing = 10.0f;

                    textWidth = 0.f; 
                    for (auto* i :helpTexts){
                        textWidth = my_Max(textWidth, fonsTextBounds(FontStash_Context, 0, 0, i, 0, NULL));
                    } 

                    glUseProgram(Flat_Shader->shaderProgram);

                    vert[0].x = width - spacing - textWidth; vert[0].y = height - spacing - textBoxHeight;
                    vert[1].x = width - spacing - textWidth; vert[1].y = height - spacing;
                    vert[2].x = width - spacing;             vert[2].y = height - spacing;
                    vert[3].x = width - spacing;             vert[3].y = height - spacing - textBoxHeight;

                    glBindBuffer(GL_ARRAY_BUFFER, Edit_Mode_Data->vbos[ptr]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, 4 * sizeof(vertex), vert);
                    glBindVertexArray(Edit_Mode_Data->vaos[ptr++]);
                    glDrawRangeElements(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, NULL);

                    glUseProgram(UI_Shader->shaderProgram);

                    for (int i = 0; i < helpTexts.size(); i++)
                    {
                        fonsDrawText(
                            FontStash_Context, 
                            width - spacing - textWidth, 
                            height - spacing - textBoxHeight + (i * (lh + 1.0f)), 
                            helpTexts[i], 
                            0);
                    }
                }
            }
        }

        // NK
        if (UI_On)
        {
            glDisable(GL_CULL_FACE);
            glEnable(GL_SCISSOR_TEST);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(UI_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glViewport(0, 0, (s32)width, (s32)height);

            {
                const struct nk_draw_command *cmd;
                void *vertices, *elements;
                nk_draw_index *offset = 0;

                glBindVertexArray(NK_Device->vao);
                glBindBuffer(GL_ARRAY_BUFFER, NK_Device->vbo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NK_Device->ebo);

#define MAX_VERTEX_MEMORY KiloByte(512)
#define MAX_ELEMENT_MEMORY KiloByte(128)
                glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

                vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

                {
                    nk_convert_config config;
                    static const nk_draw_vertex_layout_element vertex_layout[] = {
                        {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_glfw_vertex, position)},
                        {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(nk_glfw_vertex, uv)},
                        {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(nk_glfw_vertex, col)},
                        {NK_VERTEX_LAYOUT_END}
                    };

                    NK_MEMSET(&config, 0, sizeof(config));
                    config.vertex_layout = vertex_layout;
                    config.vertex_size = sizeof(nk_glfw_vertex);
                    config.vertex_alignment = NK_ALIGNOF(nk_glfw_vertex);
                    config.null = NK_Device->null;
                    config.circle_segment_count = 22;
                    config.curve_segment_count = 22;
                    config.arc_segment_count = 22;
                    config.global_alpha = 1.0f;
                    config.shape_AA = NK_ANTI_ALIASING_ON;
                    config.line_AA = NK_ANTI_ALIASING_ON;

                    {
                        nk_buffer vbuf, ebuf;
                        nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
                        nk_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
                        nk_convert(NK_Context, &NK_Device->cmds, &vbuf, &ebuf, &config);
                    }
                }
                glUnmapBuffer(GL_ARRAY_BUFFER);
                glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

                nk_draw_foreach(cmd, NK_Context, &NK_Device->cmds)
                {
                    if (!cmd->elem_count) continue;
                    glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
                    glScissor(
                            (GLint)(cmd->clip_rect.x),
                            (GLint)(height - cmd->clip_rect.y - cmd->clip_rect.h),
                            (GLint)(cmd->clip_rect.w),
                            (GLint)(cmd->clip_rect.h));
                    glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
                    offset += cmd->elem_count;
                }
            }

            ChangeSize((s32)width, (s32)height);
            glDisable(GL_SCISSOR_TEST);
            glEnable(GL_CULL_FACE);

            nk_clear(NK_Context);
        }

        if (Loading)
        {
            u32 colour = glfonsRGBA(Theme_Colour.r, Theme_Colour.g, Theme_Colour.b, Theme_Colour.a);

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, 64.0f * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsSetColor(FontStash_Context, colour);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glViewport(0, 0, (s32)width, (s32)height);
            fonsDrawText(FontStash_Context, width * 0.5f, height * 0.5f, "Loading...", 0);

            ChangeSize((s32)width, (s32)height);
        }


        if (auto_sort_state)
        {
            u32 colour = glfonsRGBA(Theme_Colour.r, Theme_Colour.g, Theme_Colour.b, Theme_Colour.a);

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, 64.0f * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsSetColor(FontStash_Context, colour);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glViewport(0, 0, (s32)width, (s32)height);
            fonsDrawText(FontStash_Context, width * 0.5f, height * 0.5f, "Pixel Sorting...", 0);

            ChangeSize((s32)width, (s32)height);
        }

        if (auto_cut_state)
        {
            u32 colour = glfonsRGBA(Theme_Colour.r, Theme_Colour.g, Theme_Colour.b, Theme_Colour.a);

            fonsClearState(FontStash_Context);
            fonsSetSize(FontStash_Context, 64.0f * Screen_Scale.x);
            fonsSetAlign(FontStash_Context, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);
            fonsSetFont(FontStash_Context, Font_Bold);
            fonsSetColor(FontStash_Context, colour);

            glUseProgram(UI_Shader->shaderProgram);
            glUniformMatrix4fv(Flat_Shader->matLocation, 1, GL_FALSE, textNormalMat);
            glViewport(0, 0, (s32)width, (s32)height);
            fonsDrawText(FontStash_Context, width * 0.5f, height * 0.5f, "Pixel cut...", 0);

            ChangeSize((s32)width, (s32)height);
        }
    }
}


global_variable
file_atlas_entry *
File_Atlas;

global_function
void
LoadTexture(void *in)
{   // 
    GLuint *textureHandle = (GLuint *)in;

    texture_buffer *buffer = TakeTextureBufferFromQueue_Wait(Texture_Buffer_Queue);  // take a buffer from the queue
    // assign the texture handle to the buffer
    buffer->x = (u16)(*textureHandle >> 16); // the former 16 bits represent the row number
    buffer->y = (u16)(*textureHandle & ((1 << 16) - 1)); // the lower 16 bits represnet the column number

    /* 
    rule for linear ordering
    Ordering of the texture boxes
        ========================
        00 01 02 03 04 ... 30 31
        // 32 33 34 35 ... 61 62
        // // 63 64 65 ... 91 92
        // // // ...
        ========================
    */
    // u32 linearIndex =  (buffer->x * (Number_of_Textures_1D - 1)) - ((buffer->x * (buffer->x-1)) >> 1) + buffer->y;  // get the index accoding to index in x and y diretion 
    u32 linearIndex = texture_id_cal((u32)buffer->x, (u32)buffer->y, (u32)Number_of_Textures_1D); 

    file_atlas_entry *entry = File_Atlas + linearIndex;  // set a tempory pointer 
    u32 nBytes = entry->nBytes;                          // base is the beginning, nBytes is the size 
    fseek(buffer->file, entry->base, SEEK_SET);          // move from the begining to the start of this texture numbered as linearIndex

    fread(buffer->compressionBuffer, 1, nBytes, buffer->file);  // read texture to the buffer

    if (
        libdeflate_deflate_decompress(
            buffer->decompressor,                     // compresser object
            (const void *)buffer->compressionBuffer,  // buffer data before decompressing
            nBytes,                                   // size before decompressing 
            (void *)buffer->texture,                  // place to store the data after decompressing
            Bytes_Per_Texture,                        // size after decompressing
            NULL)
        )
    { // 解压压缩的texture到 buffer->texture
        fprintf(stderr, "Could not decompress texture from disk\n");
    }

    while (true)
    {
        FenceIn(u32 texture_index_tmp = static_cast<u32>(Texture_Ptr));
        if (linearIndex == texture_index_tmp) break;
    }

    FenceIn(Current_Loaded_Texture = buffer); // assign buffer to current_loaded_texture
    
}

global_function
void
PopulateTextureLoadQueue(void *in) // 填充已经初始化过的 所有的queue，填充的任务为所有的528个texture的读取任务
{
    u32 *packedTextureIndexes = (u32 *)in;  // 将空指针转化为u32
    u32 ptr = 0;
    ForLoop(Number_of_Textures_1D) // row number
    {
        for (u32 index2 = index; index2 < Number_of_Textures_1D; index2++ ) // column number
        {
            packedTextureIndexes[ptr] = (index << 16) | (index2); // first 16 bits is the raw number and the second 16 bits is the column number, there will be problem if number of texture in one dimension is larger than 2^16, but it is not possible ... just for a reminder
            ThreadPoolAddTask(Thread_Pool, LoadTexture, (void *)(packedTextureIndexes + ptr++)); // 填充当前的任务队列，输入为对应的texture的行、列编号
        }
    }
}


/*
adjust the gamma thus adjust the contrast
*/
global_function
void
AdjustColorMap(s32 dir)
{
    f32 unit = 0.05f;
    f32 delta = unit * (dir > 0 ? 1.0f : -1.0f);

    Color_Maps->controlPoints[1] = my_Max(
        my_Min(Color_Maps->controlPoints[1] + delta, Color_Maps->controlPoints[2]), 
        Color_Maps->controlPoints[0]
    );

    glUseProgram(Contact_Matrix->shaderProgram);
    glUniform3fv( Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
}


/*
change to the last or next color map
*/
global_function
void
NextColorMap(s32 dir)
{
    glActiveTexture(GL_TEXTURE1);
    
    if (useCustomOrder)
    {
        // Find the current index in the custom order
        u32 currentIndex = 0;
        for (u32 i = 0; i < userColourMapOrder.nMaps; i++)
        {
            if (userColourMapOrder.order[i] == Color_Maps->currMap)
            {
                currentIndex = i;
                break;
            }
        }

        // Calculate the next index in the custom order
        u32 nextIndex = dir > 0 ? (currentIndex == (userColourMapOrder.nMaps - 1) ? 0 : currentIndex + 1) : (currentIndex == 0 ? userColourMapOrder.nMaps - 1 : currentIndex - 1);

        Color_Maps->currMap = userColourMapOrder.order[nextIndex];
    }
    else
    {
        Color_Maps->currMap = dir > 0 ? (Color_Maps->currMap == (Color_Maps->nMaps - 1) ? 0 : Color_Maps->currMap + 1) : (Color_Maps->currMap == 0 ? Color_Maps->nMaps - 1 : Color_Maps->currMap - 1);
    }
    
    glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->currMap]);
    
    glActiveTexture(GL_TEXTURE0);
}


global_function
u32
GetOrderedColorMapIndex(u32 displayIndex)
{
    if (displayIndex < userColourMapOrder.nMaps)
    {
        return userColourMapOrder.order[displayIndex];
    }
    return displayIndex;
}


global_function void
InitializeColorMapOrder()
{
    userColourMapOrder.nMaps = Color_Maps->nMaps;
    for (u32 i = 0; i < userColourMapOrder.nMaps; i++)
    {
        userColourMapOrder.order[i] = i;
    }
}


global_variable
GLuint
Quad_EBO;

enum
load_file_result
{
    ok,
    fileErr,
    decompErr,
};

global_variable
libdeflate_decompressor *
Decompressor;

global_variable
libdeflate_compressor *
Compressor;

global_variable
u08 Magic[] = {'p', 's', 't', 'm'};

global_function
FILE *
TestFile(const char *fileName, u64 *fileSize = 0)
{
    FILE *file=0;
    {
        file = fopen(fileName, "rb");
        if (!file)
        {
#ifdef DEBUG
            fprintf(stderr, "The file is not available: \'%s\' [errno %d] \n", fileName, errno);
            exit(errno);
#endif
        }
        else
        {
            if (fileSize)
            {
                fseek(file, 0, SEEK_END);
                *fileSize = (u64)ftell(file);
                fseek(file, 0, SEEK_SET);
            }

            u08 magicTest[sizeof(Magic)];

            u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file);
            if (bytesRead == sizeof(magicTest))
            {
                ForLoop(sizeof(Magic)) // #define ForLoop(n) for (u32 index=0; index<n; index++)
                {
                    if (Magic[index] != magicTest[index]) 
                    {
                        fclose(file);
                        file = 0;
                        break;
                    }
                }
            }
            else
            {
                fclose(file);
                file = 0;
            }
        }
    }
    return(file); // 仅仅返回正确的file指针，否则返回空指针
}



global_function
void 
add_graph_to_extensions(
    memory_arena *arena, 
    const u32* graph_name, 
    const u32* graph_data)
{
    
    graph *gph = PushStructP(arena, graph);   // create the size to store the extension data
    extension_node *node = PushStructP(arena, extension_node);
    u08 *dataPlusName = PushArrayP(arena, u08, ((sizeof(u32) * Number_of_Pixels_1D) + sizeof(gph->name) )); 
    gph->data = (u32 *)(dataPlusName + sizeof(gph->name)); 
    u32 *namePtr = (u32 *)graph_name;  
    for (u32 i = 0; i < ArrayCount(gph->name); i++ )  // get the graph name
    {
        gph->name[i] = *(namePtr + i);
    }
    for (u32 i = 0; i < Number_of_Pixels_1D; i++)  // get the graph data
    {
        gph->data[i] = *(graph_data + i);
    }
    node->type = extension_graph;      // assign the gph to node
    node->extension = gph;
    AddExtension(node);     // add node to extension
    
    return ;
}



global_function
void 
push_extensions_to_opengl(memory_arena *arena, u32 added_index = 0, f32 scale=-1.f)
{   
    u32 exIndex = 0;
    // TraverseLinkedList(Extensions.head, extension_node)

    extension_node* node = Extensions.head ;
    while (added_index--)
    {
        node = node->next; // 移动到最后一个节点
        exIndex ++ ;
    }
    for ( ; node; node = node->next) {
        
        switch (node->type) 
        {
            case extension_graph:
                {
                    graph *gph = (graph *)node->extension; // get the graph ptr and assigned to gph
#define DefaultGraphScale 0.2f
#define DefaultGraphBase 32.0f
#define DefaultGraphLineSize 1.0f
#define DefaultGraphColour {0.1f, 0.8f, 0.7f, 1.0f}
                    gph->scale = scale>0.f? scale : DefaultGraphScale;
                    gph->base = DefaultGraphBase;
                    gph->lineSize = DefaultGraphLineSize;
                    gph->colour = DefaultGraphColour;
                    gph->on = 0;

                    gph->shader = PushStructP(arena, editable_plot_shader);
                    gph->shader->shaderProgram = CreateShader(FragmentSource_EditablePlot.c_str(), VertexSource_EditablePlot.c_str(), GeometrySource_EditablePlot.c_str());

                    glUseProgram(gph->shader->shaderProgram);
                    glBindFragDataLocation(gph->shader->shaderProgram, 0, "outColor");
                    gph->shader->matLocation = glGetUniformLocation(gph->shader->shaderProgram, "matrix");
                    gph->shader->colorLocation = glGetUniformLocation(gph->shader->shaderProgram, "color");
                    gph->shader->yScaleLocation = glGetUniformLocation(gph->shader->shaderProgram, "yscale");
                    gph->shader->yTopLocation = glGetUniformLocation(gph->shader->shaderProgram, "ytop");
                    gph->shader->lineSizeLocation = glGetUniformLocation(gph->shader->shaderProgram, "linewidth");

                    glUniform1i(glGetUniformLocation(gph->shader->shaderProgram, "pixrearrangelookup"), 3);
                    glUniform1i(glGetUniformLocation(gph->shader->shaderProgram, "yvalues"), 4 + (s32)exIndex);

                    u32 nValues = Number_of_Pixels_1D;
                    auto* xValues = new f32[nValues]; // allocate memory for xValues, actually, x is the coordinate of the pixel
                    auto* yValues = new f32[nValues];
                    
                    u32 max = 0;
                    ForLoop(Number_of_Pixels_1D)
                    {
                        max = my_Max(max, gph->data[index]);
                    }

                    ForLoop(Number_of_Pixels_1D)
                    {
                        xValues[index] = (f32)index;
                        yValues[index] = (f32)gph->data[index] / (f32)max ;   // normalise the data
                    }

                    glActiveTexture(GL_TEXTURE4 + exIndex++);

                    GLuint yVal, yValTex;

                    // generate a buffer object named yVal and save data to yVal
                    glGenBuffers(1, &yVal);
                    glBindBuffer(GL_TEXTURE_BUFFER, yVal);
                    glBufferData(GL_TEXTURE_BUFFER, sizeof(f32) * nValues, yValues, GL_STATIC_DRAW);

                    // add texture and link to the buffer object
                    glGenTextures(1, &yValTex);
                    glBindTexture(GL_TEXTURE_BUFFER, yValTex);
                    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, yVal);

                    gph->shader->yValuesBuffer = yVal;
                    gph->shader->yValuesBufferTex = yValTex;
                    
                    // add the vertext data into buffer
                    glGenBuffers(1, &gph->vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, gph->vbo);
                    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * nValues, xValues, GL_STATIC_DRAW);

                    // gen the vertex array object
                    glGenVertexArrays(1, &gph->vao);
                    glBindVertexArray(gph->vao);

                    GLuint posAttrib = (GLuint)glGetAttribLocation(gph->shader->shaderProgram, "position");
                    glEnableVertexAttribArray(posAttrib);
                    glVertexAttribPointer(posAttrib, 1, GL_FLOAT, GL_FALSE, 0, 0);

                    delete[] xValues;
                    delete[] yValues;

                    glActiveTexture(GL_TEXTURE0);
                }
                break;
        }
    }

    return ;
}



global_function
u08
LoadState(u64 headerHash, char *path = 0);

global_function
load_file_result
LoadFile(const char *filePath, memory_arena *arena, char **fileName, u64 *headerHash)
{
    u64 fileSize = 0;

    FILE *file = TestFile(filePath, &fileSize); //  检查前4个字节读取到的数据， 如果为 u08 Magic[] = {'p', 's', 't', 'm'} 则通过验证，否则将指针file设置为空指针
    if (!file)    // 如果为空指针， 返回读取错误fileErr
    {
        return(fileErr);
    }
    
    FenceIn(File_Loaded = 0); 

    static u32 reload = 0;
    
    if (!reload)
    {
        reload = 1;
    }
    else // clear all the memory, in gl and the arena
    {   
        glDeleteTextures(1, &Contact_Matrix->textures);

        glDeleteVertexArrays((GLsizei)(Number_of_Textures_1D * Number_of_Textures_1D), Contact_Matrix->vaos);
        glDeleteBuffers((GLsizei)(Number_of_Textures_1D * Number_of_Textures_1D), Contact_Matrix->vbos);

        glDeleteBuffers(1, &Contact_Matrix->pixelStartLookupBuffer);
        glDeleteTextures(1, &Contact_Matrix->pixelStartLookupBufferTex);

        glDeleteBuffers(1, &Contact_Matrix->pixelRearrangmentLookupBuffer);
        glDeleteTextures(1, &Contact_Matrix->pixelRearrangmentLookupBufferTex);

        glDeleteVertexArrays((GLsizei)Grid_Data->nBuffers, Grid_Data->vaos);
        glDeleteBuffers((GLsizei)Grid_Data->nBuffers, Grid_Data->vbos);

        glDeleteVertexArrays((GLsizei)Label_Box_Data->nBuffers, Label_Box_Data->vaos);
        glDeleteBuffers((GLsizei)Label_Box_Data->nBuffers, Label_Box_Data->vbos);

        glDeleteVertexArrays((GLsizei)Scale_Bar_Data->nBuffers, Scale_Bar_Data->vaos);
        glDeleteBuffers((GLsizei)Scale_Bar_Data->nBuffers, Scale_Bar_Data->vbos);

        glDeleteVertexArrays((GLsizei)Contig_ColourBar_Data->nBuffers, Contig_ColourBar_Data->vaos);
        glDeleteBuffers((GLsizei)Contig_ColourBar_Data->nBuffers, Contig_ColourBar_Data->vbos);

        glDeleteVertexArrays((GLsizei)Scaff_Bar_Data->nBuffers, Scaff_Bar_Data->vaos);
        glDeleteBuffers((GLsizei)Scaff_Bar_Data->nBuffers, Scaff_Bar_Data->vbos);

        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
                case extension_graph:
                    {
                        graph *gph = (graph *)node->extension;
                        glDeleteVertexArrays(1, &gph->vao);
                        glDeleteBuffers(1, &gph->vbo);
                        glDeleteBuffers(1, &gph->shader->yValuesBuffer);
                        glDeleteTextures(1, &gph->shader->yValuesBufferTex);
                    }
                    break;
            }
        }

        Current_Loaded_Texture = 0;
        Texture_Ptr = 0;
        
        Mouse_Move.x = -1.0;
        Mouse_Move.y = -1.0;

        Camera_Position.x = 0.0f;
        Camera_Position.y = 0.0f;
        Camera_Position.z = 1.0f;
       
        Edit_Pixels.editing = 0;
        Global_Mode = mode_normal;

        Extensions = {};

        // delete the memory collected by new
        if (Contact_Matrix->vaos) 
        {
            delete[] Contact_Matrix->vaos; 
            Contact_Matrix->vaos = nullptr;
        }
        if (Contact_Matrix->vbos) 
        {
            delete[] Contact_Matrix->vbos; 
            Contact_Matrix->vbos = nullptr;
        }
        if (pixel_density_extension)
        {
            delete pixel_density_extension;
            pixel_density_extension = nullptr;
        }
        if (frag_cut_cal_ptr)
        {
            delete frag_cut_cal_ptr;
            frag_cut_cal_ptr = nullptr;
        }
        ResetMemoryArenaP(arena); // release all the memory allocated, avoid memory leak
        auto_curation_state.clear();
    }

    // File Contents
    {
        char *tmp = (char *)filePath;
#ifdef _WIN32
        char sep = '\\';
#else
        char sep = '/';
#endif

        while (*++tmp) {}
        while ((*--tmp != sep) && *tmp) {}

        *fileName = tmp + 1;

        u32 intBuff[16];
        PushStringIntoIntArray(intBuff, ArrayCount(intBuff), (u08 *)(*fileName)); // 将字符穿转移到intbuff数组中

        /*
        这段代码假设文件中的数据是以 4 字节整数的形式存储的，并且依次存储了 
        nBytesHeaderComp 和 nBytesHeader 两个值。通常情况下，
        这种操作用于读取文件中存储的数据头部信息或者其他固定格式的数据。
        */
        u32 nBytesHeaderComp; // 压缩后数据大小
        u32 nBytesHeader;     // 解压后数据大小
        fread(&nBytesHeaderComp, 1, 4, file);  // 从文件中读取 4 个字节的数据，并将其存储在 nBytesHeaderComp 变量中 
        fread(&nBytesHeader, 1, 4, file);

        u08 *header = PushArrayP(arena, u08, nBytesHeader);                // 从内存池中分配u08 数组，大小为 nBytesHeader
        u08 *compressionBuffer = PushArrayP(arena, u08, nBytesHeaderComp); // 从内存池中分配u08 数组，大小为 nBytesHeaderComp

        fread(compressionBuffer, 1, nBytesHeaderComp, file);  // nBytesHeaderComp个字节的压缩数据，存储到compressionBuffer
        *headerHash = FastHash64( // head的地址采用fasthash加密，作为存储文件的文件名。 compressionBuffer所指向的值进行hash得到一个名字，作为存储文件的文件名
            compressionBuffer, 
            nBytesHeaderComp,  
            FastHash64(intBuff, sizeof(intBuff), 0xbafd06832de619c2)
            );
        // fprintf(stdout, "The headerHash is calculated accordig to the compressed head, the hash number is (%llu) and the cache file name is (%s)\n", *headerHash, (u08*) headerHash);
        if (
            libdeflate_deflate_decompress(
            Decompressor,                     // 指向 解压缩器 
            (const void *)compressionBuffer,  // 指向 即将解压的数据，(const void *)强制转换为不可修改的内存块
            nBytesHeaderComp,                 // 解压缩的字节数
            (void *)header,                   // 指向 解压缩后存储的位置，转换为通用内存块
            nBytesHeader,                     // 解压后预计大小
            NULL)                             // 表示不传递其他参数给压缩函数
        )
        {
            return(decompErr); // decompress err
        }
        FreeLastPushP(arena); // comp buffer  释放内存池（arena）中最近一次通过 PushArrayP 宏分配的内存空间
                              // 遍历内存池链表，找到最后一个内存池，并释放其最近一次分配的内存空间。防止内存泄漏

        /*
        header的格式
        ==========================
        nBytes           Contents
        --------------------------
        8             Total_Genome_Length
        4             Number_of_Original_Contigs
        -------------------
            Number_of_Original_Contigs 个 contigs 的存储规则 
        -------------------
            4      contig fracs
            64       name
        ------------------
        1           textureRes
        1           nTextRes
        1           mipMapLevels  

        */

        u64 val64;
        u08 *ptr = (u08 *)&val64;  // 获取val64存储的 八位无符号整型（u08）的指针
        ForLoop(8)
        {
            *ptr++ = *header++;    // 指针赋值给到val64的大小 -> 整个基因的长度
        }
        Total_Genome_Length = val64;

        u32 val32;
        ptr = (u08 *)&val32;
        ForLoop(4)
        {
            *ptr++ = *header++;
        }
        Number_of_Original_Contigs = val32;   // 指针赋值给到val32的值 -> contigs的数目

        // 从内存池中分配存储原始 contig 的数组内存，类型为 original_contig，数组长度为 Number_of_Original_Contigs
        Original_Contigs = PushArrayP(arena, original_contig, Number_of_Original_Contigs);
        // 分配一个存储浮点数的数组
        f32 *contigFracs = PushArrayP(arena, f32, Number_of_Original_Contigs);
        ForLoop(Number_of_Original_Contigs) // 读取 contigs fraction (f32) and name
        {

            f32 frac;
            u32 name[16];

            // 读取每个contig 对应的一个f32
            ptr = (u08 *)&frac;
            ForLoop2(4)           // 从header中读取4个字节的数据存储到frac中 
            {
                *ptr++ = *header++;
            }
            contigFracs[index] = frac; // 将这个f32 存储到 contigFracs[index] 中
            
            // 读取contig的名字
            ptr = (u08 *)name;
            ForLoop2(64)
            {
                *ptr++ = *header++;
            }
            // contig name赋值
            ForLoop2(16)
            {
                // ？？ 一个contig 的name为什么是一个长度为u32的数组
                Original_Contigs[index].name[index2] = name[index2];   // 将 u32 name[16] 给到每一个contig  的name
            }
            
            (Original_Contigs + index)->contigMapPixels = PushArrayP(arena, u32, Number_of_Pixels_1D); // 为每个contig 的 mapPixels 变量 申请内存
            (Original_Contigs + index)->nContigs = 0; 
        }

        u08 textureRes = *header++;  // 分辨率
        u08 nTextRes = *header++;    // 纹理数目
        u08 mipMapLevels = *header;  // ？？

        Texture_Resolution = Pow2(textureRes);   // 纹理分辨率 当前显示的像素点个数，1024 
        Number_of_Textures_1D = Pow2(nTextRes);  // 一维纹理数目 可以放大的次数，每一个像素点可以放大32次，
        Number_of_MipMaps = mipMapLevels;

        Number_of_Pixels_1D = Number_of_Textures_1D * Texture_Resolution; // 更新一维数据的长度

        Map_State = PushStructP(arena, map_state);                                   // 从内存池中分配一个包含 map_state 结构的内存块，并返回指向该结构的指针， 存储contigs map到图像中的数据
        Map_State->contigIds = PushArrayP(arena, u32, Number_of_Pixels_1D);          // 从内存池中分配存储 contigIds 的数组，数组长度为 Number_of_Pixels_1D
        Map_State->originalContigIds = PushArrayP(arena, u32, Number_of_Pixels_1D);  // 
        Map_State->contigRelCoords = PushArrayP(arena, u32, Number_of_Pixels_1D);    // 像素坐标
        Map_State->scaffIds = PushArrayP(arena, u32, Number_of_Pixels_1D);           // scaffID
        Map_State->metaDataFlags = PushArrayP(arena, u64, Number_of_Pixels_1D);      // 标记
        memset(Map_State->scaffIds, 0, Number_of_Pixels_1D * sizeof(u32));           // 将scaffID和metaDataFlags初始化为0
        memset(Map_State->metaDataFlags, 0, Number_of_Pixels_1D * sizeof(u64));
        f32 total = 0.0f; // 所有contig的一个浮点数的累积, finally should approximately be 1.0
        u32 lastPixel = 0;
        u32 relCoord = 0; 

        ForLoop(Number_of_Original_Contigs)  // 初始设定每个contig的每个像素点的id和局部坐标
        {
            total += contigFracs[index]; // 当前所有contig对应的浮点数的累积，包括当前contig
            u32 pixel = (u32)((f64)Number_of_Pixels_1D * (f64)total);  // 每行像素点数 * 当前占比
            
            relCoord = 0;
#ifdef RevCoords
            u32 tmp = pixel - lastPixel - 1;
#endif
            while (lastPixel < pixel)
            {
                Map_State->originalContigIds[lastPixel] = index; // 每一个像素点对应的都是当前contig的编号
                Map_State->contigRelCoords[lastPixel++] =        // 每一个像素点对应的在当前contig中的局部坐标
#ifdef RevCoords
                    tmp - relCoord++;
#else
                    relCoord++;
#endif
            }
        }
        while (lastPixel < Number_of_Pixels_1D)  // 处理数值计算导致的lastPixel小于Number_of_Pixels_1D的问题
        {
            Map_State->originalContigIds[lastPixel] = (u32)(Number_of_Original_Contigs - 1); //假设其为最后一个contig的像素点
            Map_State->contigRelCoords[lastPixel++] = relCoord++;   
        }

        Contigs = PushStructP(arena, contigs);              // 声明一个存储contigs的内存块， 其返回Contigs作为这个块的指针，实际上此处为整个genome的信息
        Contigs->contigs_arr = PushArrayP(arena, contig, Max_Number_of_Contigs); // 每一个Contigs中会有contigs (片段)，一共有Max_Number_of_Contigs多个片段，最多存放4096个contigs
        Contigs->contigInvertFlags = PushArrayP(arena, u08, (Max_Number_of_Contigs + 7) >> 3);  // (4096 + 7 ) >> 3 = 512, 声明512个u08的存储空间

        UpdateContigsFromMapState();  //  根据mapstate 跟新当前的contigs, 并且更新original_contigs里面的每个contig所包含的片段的个数和片段的中点

        u32 nBytesPerText = 0;   //  程序将一整张图分成了32*32个小格子，每一个格子被称作texture
        ForLoop(Number_of_MipMaps)
        {
            nBytesPerText += Pow2((2 * textureRes--));  // sum([2**(2*i) for i in range(10, 10-Number_of_MipMaps, -1)])/2
        }
        nBytesPerText >>= 1; // 除以 2 因为数据是经过压缩的 
        Bytes_Per_Texture = nBytesPerText;  // 一个texture 对应的字节数目 

        File_Atlas = PushArrayP(arena, file_atlas_entry, (Number_of_Textures_1D + 1) * (Number_of_Textures_1D >> 1));   // variable to store the data entry 

        u32 currLocation = sizeof(Magic) + 8 + nBytesHeaderComp;     // current localtion of the pointer = magic_check + (u32 compressed head length) +  (u32 decompressed head length) + compressed header length  
        
        // locating the pointers to data of entries
        ForLoop((Number_of_Textures_1D + 1) * (Number_of_Textures_1D >> 1)) // loop through total number of the textures
        {
            /*
            数据结构：
                - magic (4 bytes)
                - 8 bytes
                - headercomp (nBytesHeaderComp bytes)
                - a block of entry:
                    - number of bytes in u32
                    - data

            */
            file_atlas_entry *entry = File_Atlas + index; // get the temprory pointer of the entry
            u32 nBytes;      // define a u32 to save the data 
            fread(&nBytes, 1, 4, file);    // 读取四个字节, 前四个字节存储一个u32表示大小，后面的数据存储
            fseek(file, nBytes, SEEK_CUR); // 文件指针会向前移动 nBytes 个字节，SEEK_CUR在c标准库中被定义为1, after the loop, pointer file is moved to the end of the reading file 
            currLocation += 4;             // 每移动一次，currLocation 增加 4 

            entry->base = currLocation;    // asign the current location to File_Atlas + index  
            entry->nBytes = nBytes;        // asign the size to File_Atlas + index

            currLocation += nBytes;
        }

        // Extensions
        {
            u08 magicTest[sizeof(Extension_Magic_Bytes[0])];  // define a u08 array, to check the end of the file, use this to read the extensions 

            while ((u64)(currLocation + sizeof(magicTest)) < fileSize)
            {
                u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file); // reading 4 bytes from the file
                currLocation += bytesRead;
                if (bytesRead == sizeof(magicTest))
                {
                    ForLoop(ArrayCount(Extension_Magic_Bytes))
                    {
                        u08 foundExtension = 1;
                        u08 *magic = (u08 *)Extension_Magic_Bytes[index];
                        ForLoop2(sizeof(magicTest))
                        {
                            if (magic[index2] != magicTest[index2]) // magicTest is from the file, magic is from the definition // if magic this isn't the same, means no extensions found
                            {
                                foundExtension = 0;
                                break;
                            }
                        }

                        if (foundExtension)  // if extension is found
                        {
                            extension_type type = (extension_type)index; // get the type of the extension
                            u32 extensionSize = 0;
                            switch (type)
                            {
                                case extension_graph:
                                    {
                                        u32 compSize;
                                        fread(&compSize, 1, sizeof(u32), file);   // get the size of the extension data
                                        graph *gph = PushStructP(arena, graph);   // create the size to store the extension data
                                        extension_node *node = PushStructP(arena, extension_node);
                                        u08 *dataPlusName = PushArrayP(arena, u08, ((sizeof(u32) * Number_of_Pixels_1D) + sizeof(gph->name) )); // there are 1024 * 32 u32 numbers. Every single one of them represents the data on a pixel.
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
                                        gph->data = (u32 *)(dataPlusName + sizeof(gph->name));  // the first 16 u32 are the name of the extention, which can include 16*32/8=64 u08 (char).
#pragma clang diagnostic pop                                       
                                        extensionSize += (compSize + sizeof(u32));
                                        u08 *compBuffer = PushArrayP(arena, u08, compSize);
                                        fread(compBuffer, 1, compSize, file);    // read the extension data from the file pointer
                                        if (libdeflate_deflate_decompress(Decompressor, (const void *)compBuffer, compSize, (void *)dataPlusName, (sizeof(u32) * Number_of_Pixels_1D) + sizeof(gph->name), NULL))   // decompress compBuffer to dataPlusName 
                                        /* code from the libdeflate.h
                                        enum libdeflate_result {
                                            // Decompression was successful.  
                                            LIBDEFLATE_SUCCESS = 0,

                                            // Decompression failed because the compressed data was invalid,
                                            * corrupt, or otherwise unsupported.  
                                            LIBDEFLATE_BAD_DATA = 1,

                                            // A NULL 'actual_out_nbytes_ret' was provided, but the data would have
                                            * decompressed to fewer than 'out_nbytes_avail' bytes.  
                                            LIBDEFLATE_SHORT_OUTPUT = 2,

                                            // The data would have decompressed to more than 'out_nbytes_avail'
                                            * bytes.  
                                            LIBDEFLATE_INSUFFICIENT_SPACE = 3,
                                        };
                                        */
                                        {   // unsuccessful decompress
                                            FreeLastPushP(arena); // data
                                            FreeLastPushP(arena); // graph
                                            FreeLastPushP(arena); // node
                                        }
                                        else
                                        {   // successful decompress
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
                                            u32 *namePtr = (u32 *)dataPlusName;  // get a temp pointer，将原来的u08指针切换为u32指针，所指向的位置相同，只是不同的解释方式
#pragma clang diagnostic pop
                                            ForLoop2(ArrayCount(gph->name))  // get the graph name
                                            {
                                                gph->name[index2] = *(namePtr + index2);
                                            }

                                            node->type = type;      // assign the gph to node
                                            node->extension = gph;
                                            AddExtension(node);     // add node to extension
                                        }
                                        FreeLastPushP(arena); // compBuffer
                                    }
                                    break;
                            }
                            currLocation += extensionSize;
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
        
        fclose(file); // the positions and file pointers will be saved to texture_buffer_queue, which can be read in multi-thread mode
    }

    // Load Textures: reading from file and pushing into glTexture with index Contact_Matrix->textures
    {
        InitialiseTextureBufferQueue(arena, Texture_Buffer_Queue, Bytes_Per_Texture, filePath); // 初始化所有的queue texture_buffer_queue, 一共有8个queue，每个queue有8个buffer

        u32 nTextures = (Number_of_Textures_1D + 1) * (Number_of_Textures_1D >> 1);     // number of textures (528)
        // u32 *packedTextureIndexes = PushArrayP(arena, u32, nTextures);               // using a pointer of sequences of u32 as the texture index 
        u32* packedTextureIndexes = new u32[nTextures]; // (u32*) malloc(sizeof(u32) * nTextures);              // this is released so new is ok
        ThreadPoolAddTask(Thread_Pool, PopulateTextureLoadQueue, packedTextureIndexes); // multi-thread for loading the texture entries

        glActiveTexture(GL_TEXTURE0);   // 所有的子图加载在 texture 0 
        glGenTextures(1, &Contact_Matrix->textures);   // 获取一个texture 存到
        glBindTexture(GL_TEXTURE_2D_ARRAY, Contact_Matrix->textures); // 绑定到当前的texture
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, (GLint)Number_of_MipMaps - 1);

        u32 resolution = Texture_Resolution;
        ForLoop(Number_of_MipMaps) // 初始一个texture的一个维度有1024个像素点，放大后一个texture的一个维度只有32（1024 / 2**5）个像素点
        {   // 初始化所有层级mipmap level的 gl_texture_2d_array
            glCompressedTexImage3D  (
                GL_TEXTURE_2D_ARRAY,       // 指定纹理目标 例如GL_TEXTURE_3D
                (GLint)index,              // 指定纹理层级
                GL_COMPRESSED_RED_RGTC1,   // texture数据的压缩格式  Unsigned normalized 1-component only.
                (GLsizei)resolution, (GLsizei)resolution, (GLsizei)nTextures, // 纹理宽度， 高度， 深度
                0,      // border
                (GLsizei)((resolution >> 1) * resolution * nTextures),  // 每一个texture 的数据大小 （bytes），注意此处初始化了全部的 nTextures 个texture
                0); // 指向数据的指针
            resolution >>= 1;
        }
        
        u32 ptr = 0;
        printf("Loading textures...\n");
        ForLoop(Number_of_Textures_1D)
        {
            ForLoop2(Number_of_Textures_1D - index)
            {
                volatile texture_buffer *loadedTexture = 0; // 使用volatile锁放弃存取优化。如果优化（不使用volatile），该值存取会被优化，可能会存放在寄存器中，而不是每次访问该值都会从内存中读取
                while (!loadedTexture)
                {   
                    #ifndef _WIN32 // unix
                    __atomic_load(&Current_Loaded_Texture, &loadedTexture, __ATOMIC_SEQ_CST); 
                    #else // windows 
                    // loadedTexture = InterlockedCompareExchangePointer(&Current_Loaded_Texture, nullptr, nullptr);
                    loadedTexture = (texture_buffer*)InterlockedCompareExchangePointer(
                         (PVOID volatile*)&Current_Loaded_Texture, 
                         NULL, 
                         NULL);
                    #endif // __WIN32
                }  
                u08 *texture = loadedTexture->texture; // 获取loadedtexture的texture的指针
                
                // push the texture data (number_of_mipmaps) to the gl_texture_2d_array
                resolution = Texture_Resolution;
                for (   GLint level = 0;
                        level < (GLint)Number_of_MipMaps;
                        ++level )
                {
                    GLsizei nBytes = (GLsizei)(resolution * (resolution >> 1));  // 为什么每一个mipmap存储的像素点个数是 resolution ** 2 / 2 ，不应该是resolution**2吗
                    // 此处将texture的数据压入到gl中，存储为gl_texture_2d_array的对象
                    glCompressedTexSubImage3D(
                         GL_TEXTURE_2D_ARRAY,        //  target texture. Must be GL_TEXTURE_3D or GL_TEXTURE_2D_ARRAY.
                         level,                      //  level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image. 
                         0, 0, (GLint)Texture_Ptr,   //  Texture_Ptr is the index of Current_loaded_texture
                         (GLsizei)resolution, (GLsizei)resolution, 1,   // Specifies the width, height, depth of the texture subimage.
                         GL_COMPRESSED_RED_RGTC1,    // 压缩数据的格式，这就是为什么只用了一半的 （nBytes） bytes 的数据表示了 resolution * resolution 的图片
                         nBytes,                     //  the number of unsigned bytes of image data starting at the address specified by data.
                         texture                     //  a pointer to the compressed image data in memory.
                         );   // 是否此处将texture给到 GL_TEXTURE_2D_ARRAY, check the doc on https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glCompressedTexSubImage3D.xhtml

                    resolution >>= 1;
                    texture += nBytes;
                }

                printf("\r%3d/%3d (%1.2f%%) textures loaded from disk...", Texture_Ptr + 1, nTextures, 100.0 * (f64)((f32)(Texture_Ptr + 1) / (f32)((Number_of_Textures_1D >> 1) * (Number_of_Textures_1D + 1)))); // echo out 读取到了第Texture_Ptr个texture
                fflush(stdout);

                AddTextureBufferToQueue(Texture_Buffer_Queue, (texture_buffer *)loadedTexture);  // texture_buffer_queue 是全部的读取任务队列，读取后的buffer重新添加到队列中，供下一次读取。解决了从队列中弹出任务后任务队列空了的疑问。读取文件的任务队列在别的地方还会被调用吗？
                FenceIn(Current_Loaded_Texture = 0); // 将临时变量置空，重新读取到current_loaded_texture后会跳出上面的循环
                __atomic_fetch_add(&Texture_Ptr, 1, 0); // 更新全局的 texture_ptr 
                ptr ++ ;
            }
        }

        printf("\n");
        CloseTextureBufferQueueFiles(Texture_Buffer_Queue); // 关闭所有的buffer_texture中的文件流指针file，并且释放解压器内存
        delete[] packedTextureIndexes; // packedTextureIndexes 返回  
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);  // 给之前已经压入的GL_TEXTURE_2D_ARRAY解除绑定
    }

    
    {   // Define Contact Matrix Vertex Data (vao, vbo) 
        glUseProgram(Contact_Matrix->shaderProgram);  

        // Contact_Matrix->vaos = PushArrayP(arena, GLuint, Number_of_Textures_1D * Number_of_Textures_1D);  //
        // Contact_Matrix->vbos = PushArrayP(arena, GLuint, Number_of_Textures_1D * Number_of_Textures_1D);  //
        Contact_Matrix->vaos =  new GLuint[ Number_of_Textures_1D * Number_of_Textures_1D]; //  (GLuint*) malloc(sizeof(GLuint) * Number_of_Textures_1D * Number_of_Textures_1D );  // TODO make sure the memory is freed before the re-allocation
        Contact_Matrix->vbos = new GLuint[ Number_of_Textures_1D * Number_of_Textures_1D]; // (GLuint*) malloc(sizeof(GLuint) * Number_of_Textures_1D * Number_of_Textures_1D );

        setContactMatrixVertexArray(Contact_Matrix, false, false);  // set the vertex data of the contact matrix

    }

    // Texture Pixel Lookups  texture像素查找
    {   
        GLuint pixStart, pixStartTex, pixRearrage, pixRearrageTex;
       
        u32 nTex = (Number_of_Textures_1D + 1) * (Number_of_Textures_1D >> 1); // (32 + 1) * 32 / 2 = 528 
        u32 nPix1D = Number_of_Textures_1D * Texture_Resolution; // 32 * 1024 

        glActiveTexture(GL_TEXTURE2);  // 调用 glActiveTexture 函数，可以选择当前活动的纹理单元，并且后续的纹理操作都会影响到该纹理单元

        u32 *pixStartLookup = PushArrayP(arena, u32, 2 * nTex); // 申请空间 2 * 528 个 u32
        u32 ptr = 0;
        for (u32 i = 0; i < Number_of_Textures_1D; i ++ ) // 遍历每一个texture
        {
            for (u32 j = i ; j < Number_of_Textures_1D; j ++ ) 
            {
                pixStartLookup[ptr++] = (u32)(j * Texture_Resolution); // 列 * 1024   双数索引是列，单数是行
                pixStartLookup[ptr++] = (u32)(i * Texture_Resolution);            // 行 * 1024
            }
        }

        glGenBuffers(1, &pixStart); // 生成一个缓冲区对象，并将其标识符存储到 pixStart 变量中。这样，pixStart 变量就可以用于引用这个生成的缓冲区对象
        glBindBuffer(GL_TEXTURE_BUFFER, pixStart); // 缓冲区对象 pixStart 就会被绑定到当前的纹理缓冲区上，后续的操作会影响这个缓冲区对象
        glBufferData(GL_TEXTURE_BUFFER, sizeof(u32) * 2 * nTex, pixStartLookup, GL_STATIC_DRAW);  // 将数据从 pixStartLookup 指向的内存区域拷贝到绑定到 GL_TEXTURE_BUFFER 目标的缓冲区对象中，大小为 sizeof(u32) * 2 * nTex 字节，并且告诉 OpenGL 这些数据是静态的，不会频繁地变化

        glGenTextures(1, &pixStartTex);  // 生成一个纹理对象，并将其标识符存储到 pixStartTex 变量中
        glBindTexture(GL_TEXTURE_BUFFER, pixStartTex); // 纹理对象 pixStartTex 就会被绑定到当前的纹理缓冲区上，后续的纹理操作（比如使用 glTexBuffer 函数将其与纹理缓冲区对象关联）将会影响到这个纹理对象
        glTexBuffer(  // 纹理缓冲区对象与缓冲区对象关联的函数
            GL_TEXTURE_BUFFER,  // 要关联到缓冲区的纹理目标，这里是 GL_TEXTURE_BUFFER，表示纹理缓冲区。
            GL_RG32UI,     // 纹理缓冲区数据的格式， GL_RG32UI表示每个像素由两个u32组成，一个红色分量和一个绿色分量
            pixStart);  //  缓冲区对象的标识符，这个缓冲区会与纹理缓冲区关联

        Contact_Matrix->pixelStartLookupBuffer = pixStart;       // 缓冲区对象标识符 
        Contact_Matrix->pixelStartLookupBufferTex = pixStartTex; // 纹理对象标识符

        FreeLastPushP(arena); // pixStartLookup  释放存放像素点开始的空间

        glActiveTexture(GL_TEXTURE3); // 激活第三个texture，以下代码会影响第三个texture

        u32 *pixRearrageLookup = PushArrayP(arena, u32, nPix1D); // allocte 1024 * 32 u32 memory
        for (u32 i = 0 ; i < nPix1D; i ++ )
        {
            pixRearrageLookup[i] = (u32)i;  // assign the index to the pixRearrageLookup
        }

        glGenBuffers(1, &pixRearrage);  // generate a buffer object
        glBindBuffer(GL_TEXTURE_BUFFER, pixRearrage); // bind as a texture buffer for the current buffer
        glBufferData(GL_TEXTURE_BUFFER, sizeof(u32) * nPix1D, pixRearrageLookup, GL_DYNAMIC_DRAW); // copy the data from pixRearrageLookup to the buffer object, and tell OpenGL that the data is dynamic and will change frequently

        glGenTextures(1, &pixRearrageTex); 
        glBindTexture(GL_TEXTURE_BUFFER, pixRearrageTex);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, pixRearrage);

        Contact_Matrix->pixelRearrangmentLookupBuffer = pixRearrage; // 32 * 1024 u32, this is a ascending order of 0 to 1024 * 32
        Contact_Matrix->pixelRearrangmentLookupBufferTex = pixRearrageTex; // texture 的索引

        FreeLastPushP(arena); // free pixRearrageLookup as it has already been copied to the buffer object with index pixRearrage

        glUniform1ui(glGetUniformLocation(Contact_Matrix->shaderProgram, "pixpertex"), Texture_Resolution); // 将1024传递给 pixpertex，每个texture有多少个像素点
        glUniform1f(glGetUniformLocation(Contact_Matrix->shaderProgram, "oopixpertex"), 1.0f / (f32)Texture_Resolution); // 将 1 / 1024 传递给 oopixpertex 
        glUniform1ui(glGetUniformLocation(Contact_Matrix->shaderProgram, "ntex1dm1"), Number_of_Textures_1D - 1); // 将31传递给 ntex1dm1 

        glActiveTexture(GL_TEXTURE0); //  关闭激活的texture 3
    }

    GLuint posAttribFlatShader = (GLuint)glGetAttribLocation(Flat_Shader->shaderProgram, "position");
    u32 pad = 0;
    auto PushGenericBuffer = [posAttribFlatShader, pad, arena] (quad_data **quadData, u32 numberOfBuffers)
    {
        (void)pad;

        *quadData = PushStructP(arena, quad_data);

        (*quadData)->vaos = PushArrayP(arena, GLuint, numberOfBuffers);
        (*quadData)->vbos = PushArrayP(arena, GLuint, numberOfBuffers);
        (*quadData)->nBuffers = numberOfBuffers;

        glUseProgram(Flat_Shader->shaderProgram);

        ForLoop(numberOfBuffers)
        {
            glGenVertexArrays(1, (*quadData)->vaos + index);
            glBindVertexArray((*quadData)->vaos[index]);

            glGenBuffers(1, (*quadData)->vbos + index);
            glBindBuffer(GL_ARRAY_BUFFER, (*quadData)->vbos[index]);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), NULL, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(posAttribFlatShader);
            glVertexAttribPointer(posAttribFlatShader, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad_EBO);
        }
    };

    // Grid Data
    {
        PushGenericBuffer(&Grid_Data, 2 * (Max_Number_of_Contigs + 1));
    }

    // Label Box Data
    {
        PushGenericBuffer(&Label_Box_Data, 2 * Max_Number_of_Contigs);
    }

    //Scale Bar Data
    {
        PushGenericBuffer(&Scale_Bar_Data, 4 * (2 + MaxTicksPerScaleBar));
    }

    //Contig Colour Bars
    {
        PushGenericBuffer(&Contig_ColourBar_Data, Max_Number_of_Contigs);
    }

    //Scaff Bars
    {
        PushGenericBuffer(&Scaff_Bar_Data, Max_Number_of_Contigs);
    }

    // Extensions
    {
        push_extensions_to_opengl(arena, 0);
    }

#ifdef Internal
    {
        PushGenericBuffer(&Texture_Tile_Grid, 2 * (Number_of_Textures_1D + 1));
        PushGenericBuffer(&QuadTree_Data, 1 << (2 * (Waypoints_Quadtree_Levels + 1)));
    }
#endif
    
    // Map Editor
    {
        Map_Editor = PushStructP(arena, map_editor);
        Map_Editor->nEdits = 0;
        Map_Editor->editStackPtr = 0;
        Map_Editor->nUndone = 0;
        Map_Editor->edits = PushArrayP(arena, map_edit, Edits_Stack_Size);
    }

    // Waypoint Editor
    {
        Waypoint_Editor = PushStructP(arena, waypoint_editor);
        Waypoint_Editor->nWaypointsActive = 0;
        Waypoint_Editor->freeWaypoints = {};
        Waypoint_Editor->activeWaypoints = {};
        Waypoint_Editor->freeWaypoints.next = PushArrayP(arena, waypoint, Waypoints_Stack_Size);

        Waypoint_Editor->quadtree = PushQuadTree(arena);
        Waypoint_Editor->freeNodes = {};
        Waypoint_Editor->freeNodes.next = PushArrayP(arena, waypoint_quadtree_node, Waypoints_Stack_Size);

        Waypoint_Editor->freeWaypoints.next->prev = &Waypoint_Editor->freeWaypoints;
        Waypoint_Editor->freeNodes.next->prev = &Waypoint_Editor->freeNodes;
        ForLoop(Waypoints_Stack_Size - 1)
        {
            waypoint *wayp = Waypoint_Editor->freeWaypoints.next + index;
            wayp->next = (wayp + 1);
            (wayp + 1)->prev = wayp;
            (wayp + 1)->next = 0;

            waypoint_quadtree_node *node = Waypoint_Editor->freeNodes.next + index;
            node->next = (node + 1);
            (node + 1)->prev = node;
            (node + 1)->next = 0;
        }
    }

    // 为添加的功能初始化指针
    {
        // 给pixel_density_extension分配内存
        // pixel_density_extension = new Extension_Graph_Data(Number_of_Pixels_1D);
        
        // 为 copy textures to cpu 分配内存
        if (textures_array_ptr) delete textures_array_ptr;
        textures_array_ptr = new TexturesArray4AI(
            Number_of_Textures_1D, 
            Texture_Resolution, 
            *fileName, 
            Contigs
        );
        // prepare before reading textures
        f32 original_color_control_points[3];
        prepare_before_copy(original_color_control_points);
        textures_array_ptr->copy_buffer_to_textures(
            Contact_Matrix, 
            false
        ); 
        restore_settings_after_copy(original_color_control_points);

        // 为 Pixel Cut 存储的数据分配空间，初始化，并且将数据push到extension中
        if (frag_cut_cal_ptr) delete frag_cut_cal_ptr;
        frag_cut_cal_ptr = new FragCutCal(
            textures_array_ptr,
            Number_of_Pixels_1D,
            auto_curation_state.auto_cut_diag_window_for_pixel_mean);

        // clear mem for textures
        textures_array_ptr->clear_textures();
        
        // 添加数据到extensions
        std::string graph_name = "pixel_discontinuity";
        bool is_pix_density_added = Extensions.is_graph_name_exist(graph_name);
        if (!is_pix_density_added) // 未添加pixel density
        {
            u32 added_num = Extensions.get_num_extensions();
            u32* graph_data = new u32[Number_of_Pixels_1D];
            f32 max_desity = 0.f;
            for (auto& i : frag_cut_cal_ptr->hic_pixel_density_origin) max_desity = std::max(max_desity, i);
            for (u32 i=0; i < Number_of_Pixels_1D; i ++)
            {
                graph_data[i] = (u32)(255 * (1.001f- frag_cut_cal_ptr->hic_pixel_density_origin[i] / max_desity));
            }
            add_graph_to_extensions(arena, (u32*)graph_name.c_str(), graph_data);
            delete[] graph_data;
            
            push_extensions_to_opengl(arena, added_num, 0.05f); // push hic_pixel_density to opengl buffer
        }
        else 
        {
            fmt::print("hic_pixel_density extension already exists\n");
            assert(false);
        }
    }

    FenceIn(File_Loaded = 1);

    if (LoadState(*headerHash)) LoadState(*headerHash + 1);
    return(ok);
}

global_variable
memory_arena *
Loading_Arena;

global_function
void
SetTheme(struct nk_context *ctx, enum theme theme)
{
    struct nk_color table[NK_COLOR_COUNT];
    u32 themeSet = 1;

    switch (theme)
    {
        case THEME_WHITE:
            {
                table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
                table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
                table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
                table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
                table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
                table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
                table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
                table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
                table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
                table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
                table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
                table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
                table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
                table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
                table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
                table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
                table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
                table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
                table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
                table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
            }
            break;

        case THEME_RED:
            {
                table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
                table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
                table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
                table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
                table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
                table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
                table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
                table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
                table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
                table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
                table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
                table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
                table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
                table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
                table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
                table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
                table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
                table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
                table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
            }
            break;

        case THEME_BLUE:
            {
                table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
                table[NK_COLOR_WINDOW] = nk_rgba(202, 212, 214, 215);
                table[NK_COLOR_HEADER] = nk_rgba(137, 182, 224, 220);
                table[NK_COLOR_BORDER] = nk_rgba(140, 159, 173, 255);
                table[NK_COLOR_BUTTON] = nk_rgba(137, 182, 224, 255);
                table[NK_COLOR_BUTTON_HOVER] = nk_rgba(142, 187, 229, 255);
                table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(147, 192, 234, 255);
                table[NK_COLOR_TOGGLE] = nk_rgba(177, 210, 210, 255);
                table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(182, 215, 215, 255);
                table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(137, 182, 224, 255);
                table[NK_COLOR_SELECT] = nk_rgba(177, 210, 210, 255);
                table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(137, 182, 224, 255);
                table[NK_COLOR_SLIDER] = nk_rgba(177, 210, 210, 255);
                table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(137, 182, 224, 245);
                table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(142, 188, 229, 255);
                table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(147, 193, 234, 255);
                table[NK_COLOR_PROPERTY] = nk_rgba(210, 210, 210, 255);
                table[NK_COLOR_EDIT] = nk_rgba(210, 210, 210, 225);
                table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
                table[NK_COLOR_COMBO] = nk_rgba(210, 210, 210, 255);
                table[NK_COLOR_CHART] = nk_rgba(210, 210, 210, 255);
                table[NK_COLOR_CHART_COLOR] = nk_rgba(137, 182, 224, 255);
                table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
                table[NK_COLOR_SCROLLBAR] = nk_rgba(190, 200, 200, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
                table[NK_COLOR_TAB_HEADER] = nk_rgba(156, 193, 220, 255);
            }
            break;

        case THEME_DARK:
            {
                table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
                table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
                table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
                table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
                table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
                table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
                table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
                table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
                table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
                table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
                table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
                table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
                table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
                table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
                table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
                table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
                table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
                table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
                table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
                table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
                table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
            }
            break;

        case THEME_BLACK:
        case THEME_COUNT:
            themeSet = 0;
    }

    if (themeSet) nk_style_from_table(ctx, table, Screen_Scale.x, Screen_Scale.y);
    else nk_style_default(ctx, Screen_Scale.x, Screen_Scale.y);

    Theme_Colour = table[NK_COLOR_BUTTON_ACTIVE];
    
    NK_Context->style.slider.show_buttons = nk_true;

    Current_Theme = theme;
}



global_function
void
Setup()
{
    Decompressor = libdeflate_alloc_decompressor();
    if (!Decompressor)
    {
        fprintf(stderr, "Could not allocate decompressor\n");
        exit(1);
    }

    Compressor = libdeflate_alloc_compressor(12);
    if (!Compressor)
    {
        fprintf(stderr, "Could not allocate compressor\n");
        exit(1);
    }

    Texture_Buffer_Queue = PushStruct(Working_Set, texture_buffer_queue);

    Meta_Data = PushStruct(Working_Set, meta_data);
    memset(Meta_Data, 0, sizeof(meta_data));
    MetaData_Active_Tag = 0;
    ForLoop(ArrayCount(Default_Tags)) strcpy((char *)Meta_Data->tags[MetaData_Active_Tag + index], Default_Tags[index]);
    if (Grey_Out_Settings) 
    {
        delete Grey_Out_Settings;
        Grey_Out_Settings = nullptr;
    }
    Grey_Out_Settings = new GreyOutSettings(Meta_Data);
    if (user_profile_settings_ptr)
    {
        delete user_profile_settings_ptr;
        user_profile_settings_ptr = nullptr;
    }
    user_profile_settings_ptr = new UserProfileSettings();

    // Contig Name Label UI
    {
        Contig_Name_Labels = PushStruct(Working_Set, ui_colour_element_bg);
        Contig_Name_Labels->on = 0;
        Contig_Name_Labels->fg = Yellow_Text_Float;
        Contig_Name_Labels->bg = Grey_Background;
#define DefaultNameLabelTextSize 32.0f
        Contig_Name_Labels->size = DefaultNameLabelTextSize;
    }
    
    // Scale Bar UI
    {
        Scale_Bars = PushStruct(Working_Set, ui_colour_element_bg);
        Scale_Bars->on = 0;
        Scale_Bars->fg = Red_Text_Float;
        Scale_Bars->bg = Grey_Background;
#define DefaultScaleBarSize 20.0f
        Scale_Bars->size = DefaultScaleBarSize;
    }
   
    // Grid UI
    {
        Grid = PushStruct(Working_Set, ui_colour_element);
        Grid->on = 1;
        Grid->bg = Grey_Background;
#define DefaultGridSize 0.00025f
        Grid->size = DefaultGridSize;
    }

    // Contig Ids UI
    {
        Contig_Ids = PushStruct(Working_Set, ui_colour_element);
        Contig_Ids->on = 1;
#define DefaultContigIdSize (DefaultGridSize * 3.0f)
        Contig_Ids->size = DefaultContigIdSize;
    }

    // Tool Tip UI
    {
        Tool_Tip = PushStruct(Working_Set, ui_colour_element_bg);
        Tool_Tip->on = 1;
        Tool_Tip->fg = Yellow_Text_Float;
        Tool_Tip->bg = Grey_Background;
#define DefaultToolTipTextSize 20.0f
        Tool_Tip->size = DefaultToolTipTextSize;
    }

    // Edit Mode Colours
    {
        Edit_Mode_Colours = PushStruct(Working_Set, edit_mode_colours);
        Edit_Mode_Colours->preSelect = Green_Float;
        Edit_Mode_Colours->select = Blue_Float;
        Edit_Mode_Colours->invSelect = Red_Float;
        Edit_Mode_Colours->fg = Yellow_Text_Float;
        Edit_Mode_Colours->bg = Grey_Background;
    }

    // Waypoint Mode Colours
    {
        Waypoint_Mode_Data = PushStruct(Working_Set, waypoint_mode_data);
        Waypoint_Mode_Data->base = Red_Full;
        Waypoint_Mode_Data->selected = Blue_Full;
        Waypoint_Mode_Data->text = Yellow_Text_Float;
        Waypoint_Mode_Data->bg = Grey_Background;
        Waypoint_Mode_Data->size = DefaultWaypointSize;
    }

    // Scaff Mode Colours
    {
        Scaff_Mode_Data = PushStruct(Working_Set, meta_mode_data);
        Scaff_Mode_Data->text = Yellow_Text_Float;
        Scaff_Mode_Data->bg = Grey_Background;
        Scaff_Mode_Data->size = DefaultScaffSize;
    }

    // Meta Mode Colours
    {
        MetaData_Mode_Data = PushStruct(Working_Set, meta_mode_data);
        MetaData_Mode_Data->text = Yellow_Text_Float;
        MetaData_Mode_Data->bg = Grey_Background;
        MetaData_Mode_Data->size = DefaultMetaDataSize;
    }

    // Extension Mode Colours
    {
        Extension_Mode_Data = PushStruct(Working_Set, meta_mode_data);
        Extension_Mode_Data->text = Yellow_Text_Float;
        Extension_Mode_Data->bg = Grey_Background;
        Extension_Mode_Data->size = DefaultExtensionSize;
    }

#ifdef Internal
    {
        Tiles = PushStruct(Working_Set, ui_colour_element);
        Tiles->on = 1;
        Tiles->bg = {0.0f, 1.0f, 1.0f, 1.0f};

        QuadTrees = PushStruct(Working_Set, ui_colour_element);
        QuadTrees->on = 1;
        QuadTrees->bg = {0.0f, 1.0f, 0.0f, 1.0f};
    }
#endif

    // Contact Matrix Shader
    {
        Contact_Matrix = PushStruct(Working_Set, contact_matrix);
        Contact_Matrix->shaderProgram = CreateShader(FragmentSource_Texture.c_str(), VertexSource_Texture.c_str());

        glUseProgram(Contact_Matrix->shaderProgram);
        glBindFragDataLocation(Contact_Matrix->shaderProgram, 0, "outColor");

        Contact_Matrix->matLocation = glGetUniformLocation(Contact_Matrix->shaderProgram, "matrix");
        glUniform1i(glGetUniformLocation(Contact_Matrix->shaderProgram, "tex"), 0);
        glUniform1i(glGetUniformLocation(Contact_Matrix->shaderProgram, "colormap"), 1);
        glUniform1i(glGetUniformLocation(Contact_Matrix->shaderProgram, "pixstartlookup"), 2);
        glUniform1i(glGetUniformLocation(Contact_Matrix->shaderProgram, "pixrearrangelookup"), 3);

        glActiveTexture(GL_TEXTURE1);

        Color_Maps = PushStruct(Working_Set, color_maps);
        u32 nMaps = Number_of_Color_Maps;
        Color_Maps->maps = PushArray(Working_Set, GLuint, nMaps);
        Color_Maps->mapPreviews = PushArray(Working_Set, struct nk_image, nMaps);
        Color_Maps->nMaps = nMaps;
        Color_Maps->currMap = 1;

        ForLoop(nMaps)
        {
            u32 mapPreviewImage[256];

            GLuint tbo, tboTex, texPreview;

            glGenBuffers(1, &tbo);
            glBindBuffer(GL_TEXTURE_BUFFER, tbo);
            glBufferData(GL_TEXTURE_BUFFER, sizeof(Color_Map_Data[index]), Color_Map_Data[index], GL_STATIC_DRAW);

            glGenTextures(1, &tboTex);
            glBindTexture(GL_TEXTURE_BUFFER, tboTex);
            glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo);

            Color_Maps->maps[index] = tboTex;

            glActiveTexture(GL_TEXTURE0);

            ForLoop2(256) //TODO SIMD
            {
                mapPreviewImage[index2] =   ((u32)(Color_Map_Data[index][3 * index2] * 255.0f)) |
                    (((u32)(Color_Map_Data[index][(3 * index2) + 1] * 255.0f)) << 8) |
                    (((u32)(Color_Map_Data[index][(3 * index2) + 2] * 255.0f)) << 16) |
                    0xff000000;
            }
            glGenTextures(1, &texPreview);
            glBindTexture(GL_TEXTURE_2D, texPreview);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, mapPreviewImage);
            glGenerateMipmap(GL_TEXTURE_2D);

            Color_Maps->mapPreviews[index] = nk_image_id((s32)texPreview);

            glActiveTexture(GL_TEXTURE1);
        }
        NextColorMap(-1);

        glActiveTexture(GL_TEXTURE0);

        Color_Maps->cpLocation = glGetUniformLocation(Contact_Matrix->shaderProgram, "controlpoints");
        Color_Maps->controlPoints[0] = 0.0f;
        Color_Maps->controlPoints[1] = 0.5f;
        Color_Maps->controlPoints[2] = 1.0f;
        glUniform3fv( Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
    }

    // Flat Color Shader
    {
        Flat_Shader = PushStruct(Working_Set, flat_shader);
        Flat_Shader->shaderProgram = CreateShader(FragmentSource_Flat.c_str(), VertexSource_Flat.c_str());

        glUseProgram(Flat_Shader->shaderProgram);
        glBindFragDataLocation(Flat_Shader->shaderProgram, 0, "outColor");

        Flat_Shader->matLocation = glGetUniformLocation(Flat_Shader->shaderProgram, "matrix");
        Flat_Shader->colorLocation = glGetUniformLocation(Flat_Shader->shaderProgram, "color");
    }
    
    // Fonts
    {
        UI_Shader = PushStruct(Working_Set, ui_shader);
        UI_Shader->shaderProgram = CreateShader(FragmentSource_UI.c_str(), VertexSource_UI.c_str());
        glUseProgram(UI_Shader->shaderProgram);
        glBindFragDataLocation(UI_Shader->shaderProgram, 0, "outColor");
        glUniform1i(glGetUniformLocation(UI_Shader->shaderProgram, "tex"), 0);
        UI_Shader->matLocation = glGetUniformLocation(UI_Shader->shaderProgram, "matrix");

        FontStash_Context = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
        Font_Normal = fonsAddFontMem(FontStash_Context, "Sans Regular", FontNormal, (s32)FontNormal_Size, 0);

        if (Font_Normal == FONS_INVALID)
        {
            fprintf(stderr, "Could not add font 'DroidSerif-Regular.ttf'\n");
            exit(1);
        }
        Font_Bold = fonsAddFontMem(FontStash_Context, "Sans Bold", FontBold, (s32)FontBold_Size, 0);
        if (Font_Bold == FONS_INVALID)
        {
            fprintf(stderr, "Could not add font 'DroidSerif-Bold.ttf'\n");
            exit(1);
        }
    }

    // Quad EBO
    {
        GLushort pIndexQuad[6];
        pIndexQuad[0] = 0;
        pIndexQuad[1] = 1;
        pIndexQuad[2] = 2;
        pIndexQuad[3] = 2;
        pIndexQuad[4] = 3;
        pIndexQuad[5] = 0;

        glGenBuffers(1, &Quad_EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), pIndexQuad, GL_STATIC_DRAW);
    }

    GLuint posAttribFlatShader = (GLuint)glGetAttribLocation(Flat_Shader->shaderProgram, "position");
    auto PushGenericBuffer = [posAttribFlatShader] (quad_data **quadData, u32 numberOfBuffers) -> void
    {
        *quadData = PushStruct(Working_Set, quad_data);

        (*quadData)->vaos = PushArray(Working_Set, GLuint, numberOfBuffers);
        (*quadData)->vbos = PushArray(Working_Set, GLuint, numberOfBuffers);

        glUseProgram(Flat_Shader->shaderProgram);

        ForLoop(numberOfBuffers)
        {
            glGenVertexArrays(1, (*quadData)->vaos + index);
            glBindVertexArray((*quadData)->vaos[index]);

            glGenBuffers(1, (*quadData)->vbos + index);
            glBindBuffer(GL_ARRAY_BUFFER, (*quadData)->vbos[index]);
            glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(vertex), NULL, GL_DYNAMIC_DRAW);

            glEnableVertexAttribArray(posAttribFlatShader);
            glVertexAttribPointer(posAttribFlatShader, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad_EBO);
        }
    };

    // Edit Mode Data
    {
        PushGenericBuffer(&Edit_Mode_Data, 12);
    }

    // Tool Tip Data
    {
        PushGenericBuffer(&Tool_Tip_Data, 1);
    }

    // Waypoint Data
    {
        PushGenericBuffer(&Waypoint_Data, (3 * Waypoints_Stack_Size) + 1);
    }

    // Nuklear Setup
    {
#define NK_Memory_Size MegaByte(32)
        NK_Device = PushStruct(Working_Set, device);
        u08 *nkCmdMemory = PushArray(Working_Set, u08, NK_Memory_Size);
        nk_buffer_init_fixed(&NK_Device->cmds, (void *)nkCmdMemory, NK_Memory_Size);
        NK_Device->lastContextMemory = PushArray(Working_Set, u08, NK_Memory_Size);
        memset(NK_Device->lastContextMemory, 0, NK_Memory_Size);
        NK_Device->prog = UI_Shader->shaderProgram;
        NK_Device->uniform_proj = UI_Shader->matLocation;
        NK_Device->attrib_pos = UI_SHADER_LOC_POSITION;
        NK_Device->attrib_uv = UI_SHADER_LOC_TEXCOORD;
        NK_Device->attrib_col = UI_SHADER_LOC_COLOR;

        GLsizei vs = sizeof(nk_glfw_vertex);
        size_t vp = offsetof(nk_glfw_vertex, position);
        size_t vt = offsetof(nk_glfw_vertex, uv);
        size_t vc = offsetof(nk_glfw_vertex, col);

        glGenBuffers(1, &NK_Device->vbo);
        glGenBuffers(1, &NK_Device->ebo);
        glGenVertexArrays(1, &NK_Device->vao);

        glBindVertexArray(NK_Device->vao);
        glBindBuffer(GL_ARRAY_BUFFER, NK_Device->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, NK_Device->ebo);

        glEnableVertexAttribArray((GLuint)NK_Device->attrib_pos);
        glEnableVertexAttribArray((GLuint)NK_Device->attrib_uv);
        glEnableVertexAttribArray((GLuint)NK_Device->attrib_col);

        glVertexAttribPointer((GLuint)NK_Device->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)NK_Device->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)NK_Device->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);

        NK_Atlas = PushStruct(Working_Set, nk_font_atlas);
        nk_font_atlas_init_default(NK_Atlas);
        nk_font_atlas_begin(NK_Atlas);
        struct nk_font_config cfg = nk_font_config(14);
        cfg.oversample_h = 3;
        cfg.oversample_v = 3;
        NK_Font = nk_font_atlas_add_from_memory(NK_Atlas, FontBold, (nk_size)FontBold_Size, 22 * Screen_Scale.y, &cfg);
        NK_Font_Browser = nk_font_atlas_add_from_memory(NK_Atlas, FontBold, (nk_size)FontBold_Size, 14 * Screen_Scale.y, &cfg);

        s32 w,h;
        const void *image = nk_font_atlas_bake(NK_Atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

        glGenTextures(1, &NK_Device->font_tex);
        glBindTexture(GL_TEXTURE_2D, NK_Device->font_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);

        nk_font_atlas_end(NK_Atlas, nk_handle_id((s32)NK_Device->font_tex), &NK_Device->null);
        u08 *nkContextMemory = PushArray(Working_Set, u08, NK_Memory_Size);
        nk_init_fixed(NK_Context, (void *)nkContextMemory, NK_Memory_Size, &NK_Font->handle);

        SetTheme(NK_Context, THEME_DARK);

        Theme_Name[THEME_RED] = (u08 *)"Red";
        Theme_Name[THEME_BLUE] = (u08 *)"Blue";
        Theme_Name[THEME_WHITE] = (u08 *)"White";
        Theme_Name[THEME_BLACK] = (u08 *)"Black";
        Theme_Name[THEME_DARK] = (u08 *)"Dark";
    }

    Loading_Arena = PushStruct(Working_Set, memory_arena);
    CreateMemoryArena(*Loading_Arena, MegaByte(512));
    //Loading_Arena = PushSubArena(Working_Set, MegaByte(128));
}



global_function
void
TakeScreenShot(int nchannels = 4)
{
    s32 viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    u08 *imageBuffer = PushArray(Working_Set, u08, (u32)(nchannels * viewport[2] * viewport[3]));
    glReadPixels (  0, 0, viewport[2], viewport[3],
            nchannels==4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, 
            imageBuffer);

    stbi_flip_vertically_on_write(1);
    stbi_write_png("PretextView_ScreenShot.png", viewport[2], viewport[3], nchannels, imageBuffer, nchannels * viewport[2]); //TODO change png compression to use libdeflate zlib impl
    FreeLastPush(Working_Set);
}



global_function
void
InvertMap(
    u32 pixelFrom, // from pixel
    u32 pixelTo,   // end pixel
    bool update_contigs_flag
    )
{
    if (pixelFrom > pixelTo)
    {
        u32 tmp = pixelFrom;
        pixelFrom = pixelTo;
        pixelTo = tmp;
    }

    u32 nPixels = Number_of_Pixels_1D;

    Assert(pixelFrom < nPixels);
    Assert(pixelTo < nPixels);
    
    u32 copySize = (pixelTo - pixelFrom + 1) >> 1; 
    
    u32 *tmpBuffer =  new u32[copySize];
    u32 *tmpBuffer2 = new u32[copySize];
    u32 *tmpBuffer3 = new u32[copySize];
    u32 *tmpBuffer4 = new u32[copySize];
    u64 *tmpBuffer5 = new u64[copySize];

    glBindBuffer(GL_TEXTURE_BUFFER, Contact_Matrix->pixelRearrangmentLookupBuffer);
    u32 *buffer = (u32 *)glMapBufferRange(GL_TEXTURE_BUFFER, 0, nPixels * sizeof(u32), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);  // map buffer to read and write

    if (buffer)
    {
        ForLoop(copySize) // put the first half in the buffer
        {
            tmpBuffer[index] = buffer[pixelFrom + index];
            tmpBuffer2[index] = Map_State->contigRelCoords[pixelFrom + index];
            tmpBuffer3[index] = Map_State->originalContigIds[pixelFrom + index];
            tmpBuffer4[index] = Map_State->scaffIds[pixelFrom + index];
            tmpBuffer5[index] = Map_State->metaDataFlags[pixelFrom + index];
        }

        ForLoop(copySize)  // set the first half to the second half
        {
            buffer[pixelFrom + index] = buffer[pixelTo - index];
            Map_State->contigRelCoords[pixelFrom + index] = Map_State->contigRelCoords[pixelTo - index];
            Map_State->originalContigIds[pixelFrom + index] = Map_State->originalContigIds[pixelTo - index];
            Map_State->scaffIds[pixelFrom + index] = Map_State->scaffIds[pixelTo - index];
            Map_State->metaDataFlags[pixelFrom + index] = Map_State->metaDataFlags[pixelTo - index];
        }

        ForLoop(copySize)  // set the second half from the buffer
        {
            buffer[pixelTo - index] = tmpBuffer[index];
            Map_State->contigRelCoords[pixelTo - index] = tmpBuffer2[index];
            Map_State->originalContigIds[pixelTo - index] = tmpBuffer3[index];
            Map_State->scaffIds[pixelTo - index] = tmpBuffer4[index];
            Map_State->metaDataFlags[pixelTo - index] = tmpBuffer5[index];
        }
    }
    else
    {
        fprintf(stderr, "Could not map pixel rearrange buffer\n");
    }

    glUnmapBuffer(GL_TEXTURE_BUFFER);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    delete[] tmpBuffer ;
    delete[] tmpBuffer2;
    delete[] tmpBuffer3;
    delete[] tmpBuffer4;
    delete[] tmpBuffer5;

    if (update_contigs_flag) UpdateContigsFromMapState(); // update the contigs from the buffer

    map_edit edit;
    edit.finalPix1 = (u32)pixelTo;
    edit.finalPix2 = (u32)pixelFrom;
    edit.delta = 0;
    MoveWayPoints(&edit);
}


global_function
s32
RearrangeMap(       // NOTE: VERY IMPORTANT 
    u32 pixelFrom,  // start of the fragment
    u32 pixelTo,    // end of the fragment
    s32 delta,      // movement of the distance
    u08 snap,       // if true, the fragment will snap to the nearest contig
    bool update_contigs_flag // if true, update the contigs from the map state
    )       
{

    u32 nPixels = Number_of_Pixels_1D;

    if (std::abs(delta) >= nPixels ||
        pixelFrom >= nPixels ||
        pixelTo >= nPixels)
    {
        fmt::print(
            stderr,
            "RearrangeMap: Invalid parameters: delta = {}, pixelFrom = {}, pixelTo = {},  nPixels = {}, file: {}, line: {}\n",
            delta, pixelFrom, pixelTo, nPixels, 
            __FILE__, __LINE__
        );
        assert(0);
    }

    if (pixelFrom > pixelTo)  // Swap, make sure pixelFrom is less than pixelTo
    {
        std::swap(pixelFrom, pixelTo);
    }
    u32 nPixelsInRange = pixelTo - pixelFrom + 1;

    pixelFrom += nPixels;
    pixelTo += nPixels;

    auto GetRealBufferLocation = [nPixels] (u32 index)
    {
        u32 result;

        Assert(index >= 0 && index < 3 * nPixels);

        if (index >= (2 * nPixels))
        {
            result = index - (2 * nPixels);
        }
        else if (index >= nPixels)
        {
            result = index - nPixels;
        }
        else
        {
            result = index;
        }

        return(result);
    };

    u32 forward = delta > 0;  //  move direction 

    if (snap) // can not put the selected frag into one frag.
    {
        if (forward) // move to the end
        {
            u32 target = GetRealBufferLocation(pixelTo + (u32)delta);
            u32 targetContigId = Map_State->contigIds[target] + (target == Number_of_Pixels_1D - 1 ? 1 : 0); // why is the last pixel +1?
            if (targetContigId) // only if the target is not the selected contig
            {
                contig *targetContig = Contigs->contigs_arr + targetContigId - 1;
                
                u32 targetCoord = IsContigInverted(targetContigId - 1) ? (targetContig->startCoord - targetContig->length + 1) : (targetContig->startCoord + targetContig->length - 1);
                while (delta > 0 && 
                    (Map_State->contigIds[target] != targetContigId - 1 || 
                        Map_State->contigRelCoords[target] != targetCoord))
                {
                    --target;
                    --delta;
                }
            }
            else
            {
                delta = 0;
            }
        }
        else // move to the start
        {
            u32 target = GetRealBufferLocation((u32)((s32)pixelFrom + delta));
            u32 targetContigId = Map_State->contigIds[target];
            if (targetContigId < (Contigs->numberOfContigs - 1)) // only if the target is not the selected contig
            {
                contig *targetContig = Contigs->contigs_arr + (target ? targetContigId + 1 : 0);
                u32 targetCoord = targetContig->startCoord;
                while (delta < 0 && (Map_State->contigIds[target] != (target ? targetContigId + 1 : 0) || Map_State->contigRelCoords[target] != targetCoord))
                {
                    ++target;
                    ++delta;
                }
            }
            else delta = 0;
        }
    }

    if (delta)
    {
        u32 startCopyFromRange; // location start to copy 
        u32 startCopyToRange;   // location start to put the copied data

        if (forward) // move to the end
        {
            startCopyFromRange = pixelTo + 1;
            startCopyToRange = pixelFrom;
        }
        else  // move to the start
        {
            startCopyFromRange = (u32)((s32)pixelFrom + delta);
            startCopyToRange = (u32)((s32)pixelTo + delta) + 1;
        }

        u32 copySize = std::abs(delta);

        u32 *tmpBuffer =  new u32[copySize];
        u32 *tmpBuffer2 = new u32[copySize];  // original contig ids
        u32 *tmpBuffer3 = new u32[copySize];  // original contig coords
        u32 *tmpBuffer4 = new u32[copySize];  // scaff ids
        u64 *tmpBuffer5 = new u64[copySize];  // meta data flags

        glBindBuffer(GL_TEXTURE_BUFFER, Contact_Matrix->pixelRearrangmentLookupBuffer); // pixel rearrange buffer
        u32 *buffer = (u32 *)glMapBufferRange(GL_TEXTURE_BUFFER, 0, nPixels * sizeof(u32), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT); // map the buffer

        if (buffer)
        {
            ForLoop(copySize) // copySize is abs(delta)
            {
                tmpBuffer[index] = buffer[GetRealBufferLocation(index + startCopyFromRange)]; // TODO understand how is the texture buffer data getting exchanged
                tmpBuffer2[index] = Map_State->originalContigIds[GetRealBufferLocation(index + startCopyFromRange)];
                tmpBuffer3[index] = Map_State->contigRelCoords[GetRealBufferLocation(index + startCopyFromRange)];
                tmpBuffer4[index] = Map_State->scaffIds[GetRealBufferLocation(index + startCopyFromRange)];
                tmpBuffer5[index] = Map_State->metaDataFlags[GetRealBufferLocation(index + startCopyFromRange)];
            }
            
            // copy the selected fragment to the new location
            if (forward) //  move to the ends
            {
                ForLoop(nPixelsInRange) // (pixelTo - pixelFrom + 1)
                {
                    buffer[GetRealBufferLocation(pixelTo + (u32)delta - index)] = buffer[GetRealBufferLocation(pixelTo - index)];
                    Map_State->originalContigIds[GetRealBufferLocation(pixelTo + (u32)delta - index)] = Map_State->originalContigIds[GetRealBufferLocation(pixelTo - index)];
                    Map_State->contigRelCoords[GetRealBufferLocation(pixelTo + (u32)delta - index)] = Map_State->contigRelCoords[GetRealBufferLocation(pixelTo - index)];
                    Map_State->scaffIds[GetRealBufferLocation(pixelTo + (u32)delta - index)] = Map_State->scaffIds[GetRealBufferLocation(pixelTo - index)];
                    Map_State->metaDataFlags[GetRealBufferLocation(pixelTo + (u32)delta - index)] = Map_State->metaDataFlags[GetRealBufferLocation(pixelTo - index)];
                }
            }
            else // move to the start
            {
                ForLoop(nPixelsInRange)
                {
                    buffer[GetRealBufferLocation((u32)((s32)pixelFrom + delta) + index)] = buffer[GetRealBufferLocation(pixelFrom + index)];
                    Map_State->originalContigIds[GetRealBufferLocation((u32)((s32)pixelFrom + delta) + index)] = Map_State->originalContigIds[GetRealBufferLocation(pixelFrom + index)];
                    Map_State->contigRelCoords[GetRealBufferLocation((u32)((s32)pixelFrom + delta) + index)] = Map_State->contigRelCoords[GetRealBufferLocation(pixelFrom + index)];
                    Map_State->scaffIds[GetRealBufferLocation((u32)((s32)pixelFrom + delta) + index)] = Map_State->scaffIds[GetRealBufferLocation(pixelFrom + index)];
                    Map_State->metaDataFlags[GetRealBufferLocation((u32)((s32)pixelFrom + delta) + index)] = Map_State->metaDataFlags[GetRealBufferLocation(pixelFrom + index)];
                }
            }

            // copy the influenced fragment (delta) to the new location
            ForLoop(copySize)
            {
                buffer[GetRealBufferLocation(index + startCopyToRange)] = tmpBuffer[index];
                Map_State->originalContigIds[GetRealBufferLocation(index + startCopyToRange)] = tmpBuffer2[index];
                Map_State->contigRelCoords[GetRealBufferLocation(index + startCopyToRange)] = tmpBuffer3[index];
                Map_State->scaffIds[GetRealBufferLocation(index + startCopyToRange)] = tmpBuffer4[index];
                Map_State->metaDataFlags[GetRealBufferLocation(index + startCopyToRange)] = tmpBuffer5[index];
            }
        }
        else
        {
            fprintf(stderr, "Could not map pixel rearrange buffer\n");
        }

        glUnmapBuffer(GL_TEXTURE_BUFFER);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        delete[] tmpBuffer ;
        delete[] tmpBuffer2;
        delete[] tmpBuffer3;
        delete[] tmpBuffer4;
        delete[] tmpBuffer5;
        // FreeLastPush(Working_Set); // tmpBuffer
        // FreeLastPush(Working_Set); // tmpBuffer2
        // FreeLastPush(Working_Set); // tmpBuffer3
        // FreeLastPush(Working_Set); // tmpBuffer4
        // FreeLastPush(Working_Set); // tmpBuffer5


        if (update_contigs_flag) UpdateContigsFromMapState();
        
        map_edit edit;
        edit.finalPix1 = (u32)GetRealBufferLocation((u32)((s32)pixelFrom + delta));
        edit.finalPix2 = (u32)GetRealBufferLocation((u32)((s32)pixelTo + delta));
        edit.delta = (s32)delta;
        MoveWayPoints(&edit);
    }

    return(delta);
}



global_function
void 
BreakMap(
    const u32& loc, 
    const u32 ignore_len // cut点到开头或者结尾的长度不足ignore_len的contig不会被切断
)
{  
    if (loc >= Number_of_Pixels_1D)
    {
        char buff[512];
        snprintf(buff, sizeof(buff), "[Pixel Cut] Error: loc (%d) should be smaller than number of pixels (%d)", loc, Number_of_Pixels_1D);
        MY_CHECK(buff);
        assert(0);
    }
    u32 original_contig_id = Map_State->originalContigIds[loc];
    u32 contig_id = Map_State->contigIds[loc];
    u32 contig_rel_coord = Map_State->contigRelCoords[loc];

    s32 ptr_left = (s32)loc, ptr_right = (s32)loc;
    u08 inversed = IsContigInverted(contig_id);
    while ( // 从loc向左遍历，找到第一个不满足条件的像素点索引
        --ptr_left >= 0 && 
        Map_State->contigIds[ptr_left] == contig_id && 
        (Map_State->contigRelCoords[ptr_left] ==  Map_State->contigRelCoords[ptr_left+1]+ (inversed ? +1 : -1)) ) {};
    while ( // 从loc向右遍历，找到第一个不满足条件的像素点索引
        ++ptr_right < Number_of_Pixels_1D && 
        Map_State->contigIds[ptr_right] == contig_id && 
        (Map_State->contigRelCoords[ptr_right - 1] ==  Map_State->contigRelCoords[ptr_right]+ (inversed ? +1 : -1)) ) {};
    
    if ((loc - ptr_left) < ignore_len || (ptr_right - loc) < ignore_len )  
    {   
        fmt::print(
            "[Pixel Cut] Warning: original_contig_id {} current_contig_id {}, pixel range: [{}, cut({}), {}], left({}), right({}), smaller than ignore_len ({}), cut at loc: ({}) is ignored\n", 
            original_contig_id%Max_Number_of_Contigs, 
            contig_id, 
            ptr_left, 
            loc, 
            ptr_right, 
            loc - ptr_left, 
            ptr_right - loc, 
            ignore_len, loc);
        return;
    } 

    ptr_left++; ptr_right--;

    // cut the contig by amending the original Contig Ids.
    for (u32 tmp = loc; tmp <= ptr_right; tmp++) // left side
    {   
        if (Map_State->originalContigIds[tmp] > (std::numeric_limits<u32>::max() - Max_Number_of_Contigs))
        {
            fmt::print("[Pixel Cut] originalContigIds[{}] + {} = {} is greater than the max number of contigs ({}), file: {}, line: {}\n", tmp, 
            Max_Number_of_Contigs, 
            (u64)Map_State->originalContigIds[tmp] + Max_Number_of_Contigs, 
            std::numeric_limits<u32>::max(), 
            __FILE__, __LINE__);
            assert(0);
        }
        Map_State->originalContigIds[tmp] += Max_Number_of_Contigs;  // NOTE: the number can not exceed the max number 2**32 - 1 = 4294967295. However, it is not easy to exceed the max number of contigs, as 4294967295 / 4096 = 1048575.9997
    }

    fmt::print(
        "[Pixel Cut] Original contig_id ({}), current_id ({}), pixel range: [{}, {}] {} inversed, cut at {}\n", 
        original_contig_id%Max_Number_of_Contigs, 
        contig_id, 
        ptr_left, 
        ptr_right, 
        inversed ? "(is)" : "(isn\'t)", 
        loc );

    return ; 

}


global_function
std::vector<s32>
get_exclude_metaData_idx(const std::vector<std::string>& exclude_tags)
{   
    if (exclude_tags.empty()) return std::vector<s32>();

    std::vector<s32> exclude_frag_idx((u32) exclude_tags.size(), -1);
    for (u32 i=0; i < exclude_frag_idx.size(); i++)
    {   
        for (u32 j=0; j < 64; j++)
        {   
            if (!Meta_Data->tags[j]) break;
            auto tmp = std::string((char*)Meta_Data->tags[j]);
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            if ( tmp == exclude_tags[i]) 
            {
                exclude_frag_idx[i] = j;
                break;
            }
        }
    }
    return exclude_frag_idx;
}



void RedoAllEdits(map_editor* map_editor_)
{
    while (map_editor_->nUndone) RedoMapEdit();
    return ;
}


void EraseAllEdits(map_editor* map_editor_, u32 max_edit_recorded=Edits_Stack_Size)
{
    u32 nEdits  = my_Min(max_edit_recorded, map_editor_->nEdits);
    ForLoop(nEdits) UndoMapEdit();
    return ;
}


void run_ai_detection()
{
    std::cout << "Running AI detection..." << std::endl;
    int status = std::system("/Users/sg35/miniconda3/envs/auto_cut/bin/python /Users/sg35/PretextView/python/autoCut/auto_cut.py");  

    // 检查命令执行状态
    if (status == 0) {
        std::cout << "Command successful!" << std::endl;
    } else {
        std::cerr << "Command failed, code: " << status << std::endl;
    }
}

void cut_frags(const std::vector<u32>& problem_locs)
{
    for (auto & i : problem_locs)
    {
        // cut the fragment
        BreakMap(i, auto_curation_state.auto_cut_smallest_frag_size_in_pixel);
        UpdateContigsFromMapState();
    }
    Redisplay = 1;
}


void AutoCurationFromFragsOrder(
    const FragsOrder* frags_order_,
    contigs* contigs_,
    map_state* map_state_, 
    SelectArea* select_area=nullptr) 
{   
    u08 using_select_area = (select_area && select_area->select_flag)? 1 : 0;
    u32 num_frags = contigs_->numberOfContigs;
    if (!using_select_area ) 
    {   
        if ( num_frags != frags_order_->get_num_frags())
        {
            fprintf(stderr, "Number of contigs(%d) and fragsOrder.num_frags(%d) do not match.\n", num_frags, frags_order_->get_num_frags());
            assert(0);
        }
    }
    else
    {
        if (select_area->get_to_sort_frags_num() != frags_order_->get_num_frags())
        {
            fmt::print(
                stderr, 
                "num_to_sort_contigs({}) != fragsOrder.num_frags({}), file:{}, line:{}\n", select_area->get_to_sort_frags_num(), 
                frags_order_->get_num_frags(), 
                __FILE__, __LINE__);
            assert(0);
        }
    }

    u32 num_autoCurated_edits=0;

    // check the difference between the contigs, order and the new frags order
    u32 start_loc = 0;
    std::vector<std::pair<s32, s32>> current_order(num_frags); // [contig_id, contig_length]
    std::vector<s32> predicted_order = frags_order_->get_order_without_chromosomeInfor(); // start from 1
    if (using_select_area)
    {
        std::vector<s32> full_predicted_order(num_frags);
        std::iota(full_predicted_order.begin(), full_predicted_order.end(), 1);
        auto frags_id_to_sort = select_area->get_to_sort_frags_id(Contigs);
        for (u32 i=0; i< select_area->get_to_sort_frags_num(); i++) 
            full_predicted_order[frags_id_to_sort[i]] = (predicted_order[i]>0?1:-1) * (select_area->get_first_frag_id() + std::abs(predicted_order[i]));
        predicted_order = full_predicted_order;
    }
    for (s32 i = 0; i < num_frags; ++i) current_order[i] = {i+1, contigs_->contigs_arr[i].length}; // start from 1
    auto move_current_order_element = [&current_order, &num_frags](u32 from, u32 to)
    {
        if (from >= num_frags || to >= num_frags)
        {
            fprintf(stderr, "Invalid from(%d) or to(%d), index should betwee [0, %d].\n", from, to, num_frags-1);
            return;
        }
        if (from == to) return;
        auto tmp = current_order[from];
        if (from > to) 
        {   
            for (u32 i = from; i > to; --i) current_order[i] = current_order[i-1];
        }
        else  // to > from 
        {
            for (u32 i = from; i < to; ++i) current_order[i] = current_order[i+1];
        }
        current_order[to] = tmp;
    };
    
    // update the map state based on the new order
    for (u32 i = 0; i < num_frags; ++i) 
    {
        if (predicted_order[i] == current_order[i].first)  // leave the contig unchanged
        {   
            start_loc += current_order[i].second;
            continue;
        }

        // only invert
        if (predicted_order[i] == -current_order[i].first)
        {
            u32 pixelFrom = start_loc, pixelEnd= start_loc + current_order[i].second - 1;
            InvertMap(pixelFrom, pixelEnd);
            AddMapEdit(0, {start_loc, start_loc + current_order[i].second - 1}, true);
            current_order[i].first = -current_order[i].first;
            start_loc += current_order[i].second;
            printf("[Pixel Sort] (#%d) Invert contig %d.\n", ++num_autoCurated_edits, predicted_order[i]);
            continue;
        }
        else if (
            predicted_order[i] != current_order[i].first && 
            predicted_order[i] != -current_order[i].first)  // move the contig to the new position
        {

            bool is_curated_inverted = predicted_order[i] < 0;

            // find the pixel range of the contig to move
            u32 pixelFrom = 0, tmp_i = 0; // tmp_i is the index of the current contig, which is going to be processed
            while (std::abs(predicted_order[i])!=std::abs(current_order[tmp_i].first))
            {
                if (tmp_i >= num_frags)
                {   
                    char buff[256];
                    snprintf(buff, sizeof(buff), "Error: contig %d not found in the current order.\n", current_order[i].first);
                    MY_CHECK(buff);
                    assert(0);
                }
                pixelFrom += current_order[tmp_i].second; // length updated based on the current order
                if (pixelFrom>=Number_of_Pixels_1D)
                {   
                    char buff[256];
                    snprintf(buff, sizeof(buff), "Error: pixelFrom(%d) >= Number_of_Pixels_1D(%d).\n", pixelFrom, Number_of_Pixels_1D);
                    MY_CHECK(buff);
                    assert(0);
                }
                ++tmp_i;
            }
            u32 pixelEnd = pixelFrom + current_order[tmp_i].second - 1;
            if (is_curated_inverted)
            {
                InvertMap(pixelFrom, pixelEnd);
                current_order[tmp_i].first = -current_order[tmp_i].first;
            }
            s32 delta = start_loc - pixelFrom;
            RearrangeMap(pixelFrom, pixelEnd, delta);
            AddMapEdit(delta, {(u32)((s32)pixelFrom + delta), (u32)((s32)pixelEnd + delta)}, is_curated_inverted);

            // update the current order, insert the contig to the new position
            move_current_order_element(tmp_i, i); // move element from tmp_i to i
            start_loc += current_order[i].second; // update start_loc after move fragments
            if (is_curated_inverted)
            {
                printf("[Pixel Sort] (#%d) Invert and Move contig %d to %d, start_loc:%d.\n", ++num_autoCurated_edits, predicted_order[i], i, start_loc);
            }
            else printf("[Pixel Sort] (#%d) Move contig %d to %d, start_loc:%d.\n", ++num_autoCurated_edits, predicted_order[i], i, start_loc);
            continue;
        }
        else 
        {
            fprintf(stderr, "Unknown error, predicted_order[%d] = %d, current[%d] = %d.\n", i, predicted_order[i], i, current_order[i].first);
            continue;
        }
    }
    printf("[Pixel Sort] finished with %d edits\n", num_autoCurated_edits);

    return ;
}


global_function
void
auto_cut_func(
    char * currFileName,
    memory_arena* arena = nullptr,
    std::string file_save_name = std::string("/auto_curation_tmp")
)
{   
    if (!currFileName || !auto_cut_state) return;

    if (auto_cut_state == 2) // restore the cutted frags within the selected area
    {   
        u32 start = auto_curation_state.get_start();
        u32 end = auto_curation_state.get_end();
        if (start < 0 || end < 0) 
        {
            fmt::print(
                stdout, 
                "Error: no area selected. start({}) or end({}) is less than 0\n", 
                start, end);
            return ;
        }
        fmt::print(stdout, "[Pixel cut] restoring the cutted frags within the selected area: [{}, {}]\n", start, end);
        Map_State->restore_cutted_contigs( start, end );
        UpdateContigsFromMapState();
        Redisplay = 1;
        return ;
    }

    // classic cut 
    {  
        SelectArea selected_area;
        auto_curation_state.get_selected_fragments(
            selected_area, 
            Map_State, 
            Number_of_Pixels_1D, 
            Contigs);
        glBindBuffer(GL_TEXTURE_BUFFER, Contact_Matrix->pixelRearrangmentLookupBuffer);
        const u32 *pixel_rearrange_index = (const u32 *)glMapBufferRange(GL_TEXTURE_BUFFER, 0, Number_of_Pixels_1D * sizeof(u32), GL_MAP_READ_BIT);
        std::vector<u32> cut_locs_pixel = frag_cut_cal_ptr->get_cut_locs_pixel(
            auto_curation_state, 
            pixel_rearrange_index, // pixel_rearrangement_buffer
            Contigs,
            &selected_area
        );
        glUnmapBuffer(GL_TEXTURE_BUFFER);
        
        fmt::print(stdout, "[Pixel cut] cut locs in pixel: [");
        for (auto & i : cut_locs_pixel) fmt::print(stdout, "{} ", i);
        fmt::print(stdout, "]\n");
        
        // cut the contigs
        cut_frags(cut_locs_pixel);
    }

    // generate figures
    if (0)
    {
        fprintf(stdout, "[Pixel cut] generating hic figures...\n");
        Hic_Figure hic_figure(
            Total_Genome_Length, 
            file_save_name,
            textures_array_ptr );
        hic_figure.generate_hic_figures();

        // run ai model with python script
        run_ai_detection();

        // restore the error info via reading the error json
        HIC_Problems hic_problems("/auto_curation_tmp/auto_cut_output/infer_result/infer_result.json", (f32) Total_Genome_Length / (f32)Number_of_Pixels_1D ); 
        std::vector<std::vector<u32>> problem_loc = hic_problems.get_problem_loci();
        }


    // cut the contigs 
    // cut_frags();

    return ;
}


global_function
void
auto_sort_func(char* currFileName)
{   
    if (!currFileName || !auto_sort_state) return;
    
    fprintf(stdout, "========================\n");
    fprintf(stdout, "[Pixel Sort] start...\n");
    fprintf(stdout, "[Pixel Sort] smallest_frag_size_in_pixel: %d\n", auto_curation_state.smallest_frag_size_in_pixel);
    fprintf(stdout, "[Pixel Sort] link_score_threshold:        %.3f\n", auto_curation_state.link_score_threshold);
    fprintf(stdout, "[Pixel Sort] Sort mode:                   %s\n", auto_curation_state.get_sort_mode_name().c_str());

    // compress the HiC data
    // prepare before reading textures
    f32 original_color_control_points[3];
    prepare_before_copy(original_color_control_points);
    textures_array_ptr->copy_buffer_to_textures_dynamic(Contact_Matrix, false); 
    restore_settings_after_copy(original_color_control_points);
    // check if select the area for sorting
    SelectArea selected_area;
    auto_curation_state.get_selected_fragments(
        selected_area, 
        Map_State, 
        Number_of_Pixels_1D, 
        Contigs
    );
    if (auto_curation_state.get_start() >=0 && auto_curation_state.get_end() >= 0 && selected_area.selected_frag_ids.size() > 0)
    {   
        fprintf(stdout, "[Pixel Sort] Using selected area for sorting:\n");
        fprintf(stdout, "[Pixel Sort] Selected pixel range: %d ~ %d\n", auto_curation_state.get_start(), auto_curation_state.get_end());
        std::stringstream ss; 
        ss << selected_area.selected_frag_ids[0];
        for (u32 i = 1; i < selected_area.selected_frag_ids.size(); ++i) ss << ", " << selected_area.selected_frag_ids[i];
        fmt::print(stdout, "[Pixel Sort] Selected fragments ({}): {}\n\n", selected_area.selected_frag_ids.size(), ss.str().c_str());
        selected_area.select_flag = true;
        selected_area.start_pixel = auto_curation_state.get_start();
        selected_area.end_pixel = auto_curation_state.get_end();
    }

    textures_array_ptr->cal_compressed_hic(
        Contigs, 
        Extensions, 
        false, 
        false, 
        &selected_area
    );
    FragsOrder frags_order(textures_array_ptr->get_num_frags()); // intilize the frags_order with the number of fragments including the filtered out ones
    // exclude the fragments with first two tags during the auto curation 
    std::vector<std::string> exclude_tags = {"haplotig", "unloc"};
    std::vector<s32> exclude_meta_tag_idx = get_exclude_metaData_idx(exclude_tags);
    LikelihoodTable likelihood_table(
        textures_array_ptr->get_frags(),
        textures_array_ptr->get_compressed_hic(),
        (f32)auto_curation_state.smallest_frag_size_in_pixel / ((f32)Number_of_Pixels_1D + 1.f), 
        exclude_meta_tag_idx, 
        Number_of_Pixels_1D);
    
    #ifdef DEBUG_OUTPUT_LIKELIHOOD_TABLE
        likelihood_table.output_fragsInfo_likelihoodTable("frags_info_likelihood_table.txt", textures_array_ptr->get_compressed_hic());
    #endif // DEBUG_OUTPUT_LIKELIHOOD_TABLE
    
    // clear mem for textures and compressed hic
    textures_array_ptr->clear_textures(); 
    textures_array_ptr->clear_compressed_hic();

    // use the compressed_hic to calculate the frags_order directly
    if (auto_curation_state.sort_mode == 0 || !selected_area.select_flag) // sort with union find if sorting the whole genome
    {
        frag_sort_method->sort_according_likelihood_unionFind( 
            likelihood_table, 
            frags_order, 
            selected_area,
            auto_curation_state.link_score_threshold, 
            textures_array_ptr->get_frags());
    }
    else if (auto_curation_state.sort_mode == 1)
    {
        frag_sort_method->sort_according_likelihood_unionFind_doFuse( 
            likelihood_table, 
            frags_order, 
            selected_area,
            auto_curation_state.link_score_threshold, 
            textures_array_ptr->get_frags(), true, true);
    }
    else if (auto_curation_state.sort_mode == 2)
    {
        frag_sort_method->sort_according_likelihood_unionFind_doFuse( 
            likelihood_table, 
            frags_order, 
            selected_area,
            auto_curation_state.link_score_threshold, 
            textures_array_ptr->get_frags(), false, true);
    }
    else if (auto_curation_state.sort_mode == 3) // not finished yet
    {
        frag_sort_method->sort_according_yahs(
            likelihood_table,
            frags_order,
            selected_area,
            auto_curation_state.link_score_threshold,
            textures_array_ptr->get_frags()
        );
    }
    else 
    {
        fprintf(stderr, "[Pixel Sort] Error: Unknown sort mode (%d)\n", auto_curation_state.sort_mode);
        assert(0);
    }
    AutoCurationFromFragsOrder(
        &frags_order, 
        Contigs,
        Map_State, 
        &selected_area);
    std::cout << std::endl;
    auto_sort_state = 0;

    return;
}


// set the vertex array for the contact matrix
// copy_flag: if true, set vaos to [-1, 1, -1, 1] for copying tiles from opengl buffer to cpu memory
global_function
void setContactMatrixVertexArray(
    contact_matrix* Contact_Matrix_, 
    bool copy_flag, 
    bool regenerate_flag)
{   
    glUseProgram(Contact_Matrix_->shaderProgram);
    GLuint posAttrib = (GLuint)glGetAttribLocation(Contact_Matrix_->shaderProgram, "position");
    GLuint texAttrib = (GLuint)glGetAttribLocation(Contact_Matrix_->shaderProgram, "texcoord");
    
    /*
    从左上到右下开始遍历，见上述编号
    */
    f32 x = -0.5f;
    f32 y = 0.5f;
    f32 quadSize = 1.0f / (f32)Number_of_Textures_1D; // 1.0 / 32.f = 0.03125
    f32 allCornerCoords[2][2] = {{0.0f, 1.0f}, {1.0f, 0.0f}}; 

    if (regenerate_flag) // first delete the generated vertex array objects and buffers
    {   
        glDeleteVertexArrays((GLsizei)(Number_of_Textures_1D * Number_of_Textures_1D), Contact_Matrix->vaos);
        glDeleteBuffers((GLsizei)(Number_of_Textures_1D * Number_of_Textures_1D), Contact_Matrix->vbos);
    }
    
    u32 ptr = 0;
    ForLoop(Number_of_Textures_1D)
    {
        ForLoop2(Number_of_Textures_1D)
        {
            tex_vertex textureVertices[4];

            glGenVertexArrays( // 生成对象的名称， 
                1,                            // 生成的个数
                Contact_Matrix_->vaos + ptr);  // 存储位置的指针
            glBindVertexArray(Contact_Matrix_->vaos[ptr]);  // 绑定顶点名称

            f32 *cornerCoords = allCornerCoords[index2 >= index ? 0 : 1]; // 包含对角线的上三角的时候取第一行{0, 1}，否则取第二行{1, 0}  为了解决关于对角线对称
            
            u32 min = my_Min(index, index2);
            u32 max = my_Max(index, index2);
            /*
                对称性编号
                [[ 0  1  2  3  4  5  6  7  8  9]
                    [ 1 10 11 12 13 14 15 16 17 18]
                    [ 2 11 19 20 21 22 23 24 25 26]
                    [ 3 12 20 27 28 29 30 31 32 33]
                    [ 4 13 21 28 34 35 36 37 38 39]
                    [ 5 14 22 29 35 40 41 42 43 44]
                    [ 6 15 23 30 36 41 45 46 47 48]
                    [ 7 16 24 31 37 42 46 49 50 51]
                    [ 8 17 25 32 38 43 47 50 52 53]
                    [ 9 18 26 33 39 44 48 51 53 54]]
            */
            f32 texture_index_symmetric = (f32)((min * (Number_of_Textures_1D - 1))
                - (((min-1) * min) >> 1 )
                + max) ;
            
            /*
                3 2
                0 1
            */
            f32 half_len = 1.0f;
            textureVertices[0].x = !copy_flag? x               : -half_len;   // x -> [-0.5, 0.5]
            textureVertices[0].y = !copy_flag? y - quadSize    : -half_len;   // y -> [-0.5, 0.5]
            textureVertices[0].s = !copy_flag? cornerCoords[0] :  0.f; // s表示texture的水平分量
            textureVertices[0].t = !copy_flag? cornerCoords[1] :  0.f; // t表示texture的垂直分量
            textureVertices[0].u = texture_index_symmetric;

            textureVertices[1].x = !copy_flag? x + quadSize    :  half_len;   
            textureVertices[1].y = !copy_flag? y - quadSize    : -half_len;
            textureVertices[1].s = !copy_flag? 1.0f            :  1.f;
            textureVertices[1].t = !copy_flag? 1.0f            :  0.f;
            textureVertices[1].u = texture_index_symmetric;

            textureVertices[2].x = !copy_flag? x + quadSize    :  half_len;   
            textureVertices[2].y = !copy_flag? y               :  half_len;
            textureVertices[2].s = !copy_flag? cornerCoords[1] :  1.f;
            textureVertices[2].t = !copy_flag? cornerCoords[0] :  1.f;
            textureVertices[2].u = texture_index_symmetric;

            textureVertices[3].x = !copy_flag? x               : -half_len;              
            textureVertices[3].y = !copy_flag? y               :  half_len;
            textureVertices[3].s = !copy_flag? 0.0f            :  0.f;
            textureVertices[3].t = !copy_flag? 0.0f            :  1.f;
            textureVertices[3].u = texture_index_symmetric;

            glGenBuffers(                    // 生成buffer的名字 存储在contact_matrix->vbos+ptr 对应的地址上
                1,                           // 个数
                Contact_Matrix_->vbos + ptr); // 指针，存储生成的名字  
            glBindBuffer(                    // 绑定到一个已经命名的buffer对象上
                GL_ARRAY_BUFFER,             // 绑定的对象，gl_array_buffer 一般是绑定顶点的信息
                Contact_Matrix_->vbos[ptr]);  // 通过输入buffer的名字将其绑定到gl_array_buffer
            glBufferData(                    // 创建、初始化存储数据的buffer
                GL_ARRAY_BUFFER,             // 绑定的对象，gl_array_buffer 一般是绑定顶点的信息
                4 * sizeof(tex_vertex),      // 对象的大小（bytes）
                textureVertices,             // 即将绑定到gl_array_buffer的数据的指针
                GL_STATIC_DRAW);             // 绑定后的数据是用于干什么的
            
            // tell GL how to interpret the data in the GL_ARRAY_BUFFER
            glEnableVertexAttribArray(posAttrib); 
            glVertexAttribPointer(           // 定义一个数组存储所有的通用顶点属性
                posAttrib,                   // 即将修改属性的索引
                2,                           // 定义每个属性的参数的个数， 只能为1 2 3 4中的一个，初始值为4
                GL_FLOAT,                    // 定义该属性的数据类型，则数据为4个字节， 占用两个的话，则一共为8字节
                GL_FALSE,                    // normalized， 是否要在取用的时候规范化这些数
                sizeof(tex_vertex),          // stride  定义从一个信息的指针下一个信息指针的bytes的间隔， 一个信息包到下一个信息包的长度
                0);                          // const void* 指定当前与 GL_ARRAY_BUFFER 绑定的缓冲区数据头指针偏移多少个到 第一个通用顶点属性的第一个特征开始的位置。 初始值为 0。因为此处想要读取的信息为 x 和 y
            glEnableVertexAttribArray(texAttrib); // 
            glVertexAttribPointer(
                texAttrib, 
                3, 
                GL_FLOAT,   // 定义每个属性的数据类型
                GL_FALSE,   // GL_FALSE, GL_TRUE
                sizeof(tex_vertex), 
                (void *)(3 * sizeof(GLfloat))); // 偏移量为3，因为前三个为x, y and pad, 想要读取的数据为 s, t and u

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Quad_EBO);

            x += quadSize; // next column
            ++ptr;         // ptr for current texture
        }

        y -= quadSize;  // next row
        x = -0.5f;      // from column 0
    }
}


global_function
void 
prepare_before_copy(f32 *original_color_control_points)
{
    // prepare before reading textures
    glActiveTexture(GL_TEXTURE1);

    glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->nMaps-1]); // 绑定到颜色映射的最后一个颜色
    glActiveTexture(GL_TEXTURE0);
    for (u32 i = 0; i < 3 ; i ++ ) 
        original_color_control_points[i] = Color_Maps->controlPoints[i];
    Color_Maps->controlPoints[0] = 0.0f;
    Color_Maps->controlPoints[1] = 0.5f;
    Color_Maps->controlPoints[2] = 1.0f;
    glUseProgram(Contact_Matrix->shaderProgram);
    glUniform3fv( Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
    setContactMatrixVertexArray(Contact_Matrix, true, true);  // set the vertex for copying
    return ;
}


global_function
void 
restore_settings_after_copy(const f32 *original_color_control_points)
{
    // restore settings
    setContactMatrixVertexArray(Contact_Matrix, false, true);  // restore the vertex array for rendering
    for (u32 i = 0 ; i < 3 ; i ++ )
        Color_Maps->controlPoints[i] = original_color_control_points[i];
    glUseProgram(Contact_Matrix->shaderProgram);
    glUniform3fv( Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->currMap]);
    glActiveTexture(GL_TEXTURE0);
}


global_function void
JumpToDiagonal(GLFWwindow *window)
{
    Camera_Position.x = -Camera_Position.y;
    ClampCamera();
    Redisplay = 1;
}


global_function
u32
ToggleEditMode(GLFWwindow* window)
{
    u32 result = 1;

    if (Edit_Mode && !Edit_Pixels.editing)
    {
        Edit_Pixels.scaffSelecting = 0;
        Global_Mode = mode_normal;
        if (Tool_Tip->on)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
    else if (Normal_Mode || Select_Sort_Area_Mode || MetaData_Edit_Mode)
    {
        Global_Mode = mode_edit;
    }
    else
    {
        result = 0;
    }

    return(result);
}


global_function
    u32
    ToggleExtensionMode(GLFWwindow *window)
{
    u32 result = 1;

    if (Extension_Mode)
    {
        Global_Mode = mode_normal;
    }
    else if (Normal_Mode)
    {
        Global_Mode = mode_extension;
    }
    else
    {
        result = 0;
    }

    return result;
}


global_function
u32
ToggleWaypointMode(GLFWwindow* window)
{
    u32 result = 1;

    if (Waypoint_Edit_Mode)
    {
        Global_Mode = mode_normal;
        if (Tool_Tip->on)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
    else if (Normal_Mode)
    {
        Global_Mode = mode_waypoint_edit;
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
    else
    {
        result = 0;
    }

    return(result);
}

global_function
u32
ToggleScaffMode(GLFWwindow* window)
{
    u32 result = 1;

    if (Scaff_Edit_Mode && !Scaff_Painting_Flag)
    {
        Global_Mode = mode_normal;
        Scaff_Painting_Flag = 0;
        Scaff_Painting_Id = 0;
        Scaff_FF_Flag = 0;
        if (Tool_Tip->on)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
    else if (Normal_Mode)
    {
        Global_Mode = mode_scaff_edit;
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
    else
    {
        result = 0;
    }

    return(result);
}

global_function
u32
ToggleMetaDataMode(GLFWwindow* window)
{
    u32 result = 1;

    if (MetaData_Edit_Mode)
    {
        Global_Mode = mode_normal;
        if (Tool_Tip->on)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
    else if (Normal_Mode || (Edit_Mode && !Edit_Pixels.editing) || Select_Sort_Area_Mode)
    {
        Global_Mode = mode_meta_edit;
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
    else
    {
        result = 0;
    }

    return(result);
}


global_function
u32
ToggleSelectSortAreaMode(GLFWwindow* window)
{
    u32 result = 1;

    if (Select_Sort_Area_Mode)
    {
        Global_Mode = mode_normal;
        if (Tool_Tip->on)
        {
            f64 mousex, mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            MouseMove(window, mousex, mousey);
        }
    }
    else if (Normal_Mode || (Edit_Mode && !Edit_Pixels.editing) || MetaData_Edit_Mode)
    {
        Global_Mode = mode_selectExclude_sort_area;
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
    else
    {
        result = 0;
    }

    return(result);

}



global_function
void
ToggleToolTip(GLFWwindow* window)
{
    Tool_Tip->on = !Tool_Tip->on;
    if (Tool_Tip->on)
    {
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
}

#if 0
enum
MouseButon
{
    left,
    right,
    middle,
};

struct
KeyBindings
{
    s32 ui;
    s32 editMode;
    s32 waypointMode;
    s32 undoEdit;
    s32 redoEdit;
    s32 selectWholeSeq_key;
    MouseButon selectWholeSeq_mouse;

};
#endif

global_variable
s32
Windowed_Xpos, Windowed_Ypos, Windowed_Width, Windowed_Height;

#if 0
struct
GLFWKeyAndScancode
{
    s32 key;
    s32 scancode;
};

global_variable
GLFWKeyAndScancode
LastPressedKey = {0, 0};
#endif

global_variable
nk_keys
NK_Pressed_Keys[1024] = {};

global_variable
u32
NK_Pressed_Keys_Ptr = 0;

global_variable
u32
GatheringTextInput = 0;

global_function
void
KeyBoard(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods)
{
    if (!Loading && !auto_sort_state && !auto_cut_state && (action != GLFW_RELEASE || key == GLFW_KEY_SPACE || key == GLFW_KEY_A || key == GLFW_KEY_LEFT_SHIFT))
    {
        if (UI_On)
        {
#if 0
            LastPressedKey.key = key;
            LastPressedKey.scancode = scancode;
#else
            (void)scancode;
#endif

            if (key == GLFW_KEY_ENTER && mods == GLFW_MOD_ALT)
            {
                if (glfwGetWindowMonitor(window))
                {
                    glfwSetWindowMonitor(window, NULL,
                            Windowed_Xpos, Windowed_Ypos,
                            Windowed_Width, Windowed_Height, 0);
                }
                else
                {
                    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                    if (monitor)
                    {
                        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                        glfwGetWindowPos(window, &Windowed_Xpos, &Windowed_Ypos);
                        glfwGetWindowSize(window, &Windowed_Width, &Windowed_Height);
                        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    }
                }
            }
#ifdef Internal
            else if (key == GLFW_KEY_ESCAPE && !mods) 
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
#endif
            /*nk_input_key(NK_Context, NK_KEY_DEL, glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_ENTER, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_TAB, glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_BACKSPACE, glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_LEFT, glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_RIGHT, glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_UP, glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_DOWN, glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);

            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            {
                nk_input_key(NK_Context, NK_KEY_COPY, glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_PASTE, glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_CUT, glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_CUT, glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_SHIFT, 1);
            } 
            else
            {
                nk_input_key(NK_Context, NK_KEY_COPY, 0);
                nk_input_key(NK_Context, NK_KEY_PASTE, 0);
                nk_input_key(NK_Context, NK_KEY_CUT, 0);
                nk_input_key(NK_Context, NK_KEY_SHIFT, 0);
            }*/


            else if (key == GLFW_KEY_U)
            {
                Deferred_Close_UI = 1;
            }
            else if (action != GLFW_RELEASE && !GatheringTextInput)
            {
                nk_keys addKey = NK_KEY_NONE;
                switch (key)
                {
                    case GLFW_KEY_DELETE:
                        addKey = NK_KEY_DEL;
                        break;

                    case GLFW_KEY_ENTER:
                        addKey = NK_KEY_ENTER;
                        break;

                    case GLFW_KEY_TAB:
                        addKey = NK_KEY_TAB;
                        break;

                    case GLFW_KEY_BACKSPACE:
                        addKey = NK_KEY_BACKSPACE;
                        break;

                    case GLFW_KEY_LEFT:
                        addKey = NK_KEY_LEFT;
                        break;

                    case GLFW_KEY_RIGHT:
                        addKey = NK_KEY_RIGHT;
                        break;

                    case GLFW_KEY_UP:
                        addKey = NK_KEY_UP;
                        break;

                    case GLFW_KEY_DOWN:
                        addKey = NK_KEY_DOWN;
                        break;
                }

                if (addKey != NK_KEY_NONE)
                {
                    NK_Pressed_Keys[NK_Pressed_Keys_Ptr++] = addKey;
                    if (NK_Pressed_Keys_Ptr == ArrayCount(NK_Pressed_Keys)) NK_Pressed_Keys_Ptr = 0;
                }
            }
        }
        else // not UI_On
        {
            u32 keyPressed = 1;

            switch (key)
            {   
                
                case GLFW_KEY_A:
                    if (Scaff_Edit_Mode)
                    {
                        if (action != GLFW_RELEASE) Scaff_FF_Flag |= 1;
                        else Scaff_FF_Flag &= ~1;
                    }
                    else keyPressed = 0;
                    break;

                case GLFW_KEY_B:
                    Scale_Bars->on = !Scale_Bars->on;
                    break;

                case GLFW_KEY_C:
                    if (Select_Sort_Area_Mode && auto_curation_state.get_selected_fragments_num(Map_State, Number_of_Pixels_1D) > 0) // local area break
                    {
                        auto_cut_state = 1;
                    }
                    else // open "coverage" extension
                    {
                        if (!Extensions.head) break;
                        TraverseLinkedList(Extensions.head, extension_node)
                        {
                            switch (node->type)
                            {
                            case extension_graph:
                            {

                                graph *gph = (graph *)node->extension;
                                if (strcmp((char *)gph->name, "coverage") == 0)
                                {
                                    gph->on = !gph->on;
                                    break;
                                }
                            }
                            }
                        }
                    }
                    
                    break;

                case GLFW_KEY_D:
                    if (Scaff_Edit_Mode && (mods & GLFW_MOD_SHIFT))
                    {
                        ForLoop(Contigs->numberOfContigs) (Contigs->contigs_arr + index)->scaffId = 0;
                        UpdateScaffolds();
                    }
                    else if (MetaData_Edit_Mode && (mods & GLFW_MOD_SHIFT)) memset(Map_State->metaDataFlags, 0, Number_of_Pixels_1D * sizeof(u64));
                    else keyPressed = 0;
                    break;

                case GLFW_KEY_E:
                    keyPressed = ToggleEditMode(window);
                    break;

                case GLFW_KEY_F:
                    keyPressed = ToggleSelectSortAreaMode(window);
                    break;

                case GLFW_KEY_G:
                    if (!Extensions.head) break;
                    TraverseLinkedList(Extensions.head, extension_node)
                    {
                        switch (node->type)
                        {
                        case extension_graph:
                        {

                            graph *gph = (graph *)node->extension;
                            if (strcmp((char *)gph->name, "gap") == 0)
                            {
                                gph->on = !gph->on;
                                break;
                            }
                        }
                        }
                    }
                    break;

                case GLFW_KEY_H:
                    break;

                case GLFW_KEY_I:
                    Contig_Ids->on = !Contig_Ids->on;
                    break;

                case GLFW_KEY_J:
                    JumpToDiagonal(window);
                    break;

                case GLFW_KEY_K:
                    break;

                case GLFW_KEY_L:
                    if (Waypoint_Edit_Mode)
                    {
                        Long_Waypoints_Mode = (Long_Waypoints_Mode +1) % 3; 
                    }
                    else
                    {
                        Grid->on = !Grid->on;
                    }
                    break;

                case GLFW_KEY_M:
                    keyPressed = ToggleMetaDataMode(window);
                    break;

                case GLFW_KEY_N:
                    Contig_Name_Labels->on = !Contig_Name_Labels->on;
                    break;

                case GLFW_KEY_O:
                    break;
                
                case GLFW_KEY_P:
                    break;

                case GLFW_KEY_Q:
                    if (Edit_Mode || Select_Sort_Area_Mode)
                    {
                        UndoMapEdit();
                    }
                    else
                    {
                        keyPressed = 0;
                    }
                    break;

                case GLFW_KEY_R:
                    if (mods == GLFW_MOD_CONTROL)
                    {
                        Loading = 1;
                    }
                    else if (Extension_Mode && Extensions.head)
                    {
                        TraverseLinkedList(Extensions.head, extension_node)
                        {
                            switch (node->type)
                            {
                            case extension_graph:
                            {
                                graph *gph = (graph *)node->extension;
                                if (strcmp((char *)gph->name, "repeat_density") == 0)
                                {
                                    gph->on = !gph->on;
                                    break;
                                }
                            }
                            }
                        }
                        break;
                    }
                    else
                    {
                        keyPressed = 0;
                    }
                    break;

                case GLFW_KEY_S:
                    if (Edit_Mode)
                    {
                        Edit_Pixels.snap = !Edit_Pixels.snap;
                    }
                    else if (mods & GLFW_MOD_SHIFT)
                    {
                        Scaffs_Always_Visible = Scaffs_Always_Visible ? 0 : 1;
                    }
                    else if (Select_Sort_Area_Mode)
                    {
                        auto_curation_state.clear(); // clear the selected area
                    }
                    else
                    {
                        keyPressed = ToggleScaffMode(window);
                    }
                    break;

                case GLFW_KEY_T:
                    if (Extension_Mode && Extensions.head)
                    {
                        TraverseLinkedList(Extensions.head, extension_node)
                        {
                            switch (node->type)
                            {
                            case extension_graph:
                            {

                                graph *gph = (graph *)node->extension;
                                if (strcmp((char *)gph->name, "telomere") == 0)
                                {
                                    gph->on = !gph->on;
                                    break;
                                }
                            }
                            }
                        }
                        break;
                    }
                    else
                    {
                        ToggleToolTip(window);
                    }
                    break;

                case GLFW_KEY_U:
                    UI_On = !UI_On;
                    ++NK_Device->lastContextMemory[0];
                    Mouse_Move.x = Mouse_Move.y = -1;
                    break;

                case GLFW_KEY_V:
                    break;

                case GLFW_KEY_W:
                    if (Edit_Mode || Select_Sort_Area_Mode)
                    {
                        RedoMapEdit();
                    }
                    else
                    {
                        keyPressed = ToggleWaypointMode(window);
                    }
                    break;

                case GLFW_KEY_X:
                    keyPressed = ToggleExtensionMode(window);
                    break;

                case GLFW_KEY_Y:
                    break;

                case GLFW_KEY_Z:
                    if (Select_Sort_Area_Mode)
                    {
                        auto_cut_state = 2; // restore the cutted frags within the selected area
                    }
                    break;

                case GLFW_KEY_SPACE:
                    if (Edit_Mode && !Edit_Pixels.editing && action == GLFW_PRESS)
                    {
                        Edit_Pixels.selecting = 1;
                        Edit_Pixels.selectPixels = Edit_Pixels.pixels;
                        f64 x, y;
                        glfwGetCursorPos(window, &x, &y);
                        MouseMove(window, x, y);
                    }
                    else if (Edit_Mode && !Edit_Pixels.editing && action == GLFW_RELEASE)
                    {
                        Edit_Pixels.editing = 1;
                        Edit_Pixels.selecting = 0;
                        f64 x, y;
                        glfwGetCursorPos(window, &x, &y);
                        MouseMove(window, x, y);
                    }
                    else if (Edit_Mode && Edit_Pixels.editing && action == GLFW_PRESS)
                    {
                        InvertMap(Edit_Pixels.pixels.x, Edit_Pixels.pixels.y);
                        Global_Edit_Invert_Flag = !Global_Edit_Invert_Flag;
                    }
                    else if (Waypoint_Edit_Mode && Selected_Waypoint && action == GLFW_PRESS)
                    {
                        f64 x, y;
                        glfwGetCursorPos(window, &x, &y);
                        RemoveWayPoint(Selected_Waypoint);
                        MouseMove(window, x, y);
                    }
                    else if (Scaff_Edit_Mode && (action == GLFW_PRESS || action == GLFW_RELEASE))
                    {
                        f64 x, y;
                        glfwGetCursorPos(window, &x, &y);
                        Scaff_Painting_Flag = action == GLFW_PRESS ? 2 : 0;
                        MouseMove(window, x, y);
                        if (action == GLFW_RELEASE) UpdateScaffolds();
                    }
                    else if (MetaData_Edit_Mode && (action == GLFW_PRESS || action == GLFW_RELEASE))
                    {
                        f64 x, y;
                        glfwGetCursorPos(window, &x, &y);
                        MetaData_Edit_State = action == GLFW_PRESS ? 2 : 0;
                        MouseMove(window, x, y);
                    }
                    else if (Select_Sort_Area_Mode && action == GLFW_PRESS)
                    {   
                        u32 selected_frag_num = auto_curation_state.get_selected_fragments_num(Map_State, Number_of_Pixels_1D);
                        if (selected_frag_num >= 2)
                        {
                            auto_sort_state = 1;
                        }
                    }
                    else
                    {
                        keyPressed = 0;
                    }
                    break;
                
                case GLFW_KEY_LEFT_SHIFT:
                    if (Scaff_Edit_Mode)
                    {
                        if (action != GLFW_RELEASE) Scaff_FF_Flag |= 2;
                        else Scaff_FF_Flag &= ~2;
                    }
                    else if (Edit_Mode) Edit_Pixels.scaffSelecting = action != GLFW_RELEASE;
                    else keyPressed = 0;
                    break;

                case GLFW_KEY_LEFT:
                    if (MetaData_Edit_Mode)
                    {
                        u32 nextActive = MetaData_Active_Tag;
                        ForLoop(ArrayCount(Meta_Data->tags))
                        {
                            if (--nextActive > (ArrayCount(Meta_Data->tags) - 1)) nextActive = ArrayCount(Meta_Data->tags) - 1;
                            if (strlen((const char *)Meta_Data->tags[nextActive]))
                            {
                                MetaData_Active_Tag = nextActive;
                                break;
                            }
                        }

                    }
                    else if (Select_Sort_Area_Mode)
                    {
                        auto_curation_state.change_sort_mode(-1);
                    }
                    else AdjustColorMap(-1);
                    break;

                case GLFW_KEY_RIGHT:
                    if (MetaData_Edit_Mode)
                    {
                        u32 nextActive = MetaData_Active_Tag;
                        ForLoop(ArrayCount(Meta_Data->tags))
                        {
                            if (++nextActive == ArrayCount(Meta_Data->tags)) nextActive = 0;
                            if (strlen((const char *)Meta_Data->tags[nextActive]))
                            {
                                MetaData_Active_Tag = nextActive;
                                break;
                            }
                        }
                    }
                    else if (Select_Sort_Area_Mode)
                    {
                        auto_curation_state.change_sort_mode(1);
                    }
                    else AdjustColorMap(1);
                    break;

                case GLFW_KEY_UP:
                    if (Select_Sort_Area_Mode)
                    {
                        auto_curation_state.adjust_cut_threshold(1);
                    }
                    else NextColorMap(1);
                    break;

                case GLFW_KEY_DOWN:
                    if (Select_Sort_Area_Mode)
                    {
                        auto_curation_state.adjust_cut_threshold(-1);
                    }
                    else 
                        NextColorMap(-1);
                    break;
#ifdef Internal
                case GLFW_KEY_ESCAPE:
                    if (!mods)
                    {
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }
                    break;
#endif
                case GLFW_KEY_ENTER:
                    if (mods == GLFW_MOD_ALT)
                    {
                        if (glfwGetWindowMonitor(window))
                        {
                            glfwSetWindowMonitor(window, NULL,
                                    Windowed_Xpos, Windowed_Ypos,
                                    Windowed_Width, Windowed_Height, 0);
                        }
                        else
                        {
                            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                            if (monitor)
                            {
                                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                                glfwGetWindowPos(window, &Windowed_Xpos, &Windowed_Ypos);
                                glfwGetWindowSize(window, &Windowed_Width, &Windowed_Height);
                                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                            }
                        }
                    }
                    else
                    {
                        keyPressed = 0;
                    }
                    break;

                default:
                    keyPressed = 0;
            }

            if (keyPressed)
            {
                Redisplay = 1;
            }
        }
    }
}

global_function
void
ErrorCallback(s32 error, const char *desc)
{
    (void)error;
    fprintf(stderr, "Error: %s\n", desc);
}

// File Browser
// from nuklear file browser example
/* ===============================================================
 *
 *                          GUI
 *
 * ===============================================================*/
struct
icons
{
    struct nk_image home;
    struct nk_image computer;
    struct nk_image directory;

    struct nk_image default_file;
    struct nk_image img_file;
};

enum
file_groups
{
    FILE_GROUP_DEFAULT,
    FILE_GROUP_PRETEXT,
    FILE_GROUP_MAX
};

enum
file_types
{
    FILE_DEFAULT,
    FILE_PRETEXT,
    FILE_PSTM,
    FILE_MAX
};

struct
file_group
{
    enum file_groups group;
    u32 pad;
    const char *name;
    struct nk_image *icon;
};

struct
file
{
    enum file_types type;
    enum file_groups group;
    const char *suffix;
};

struct
media
{
    int font;
    int icon_sheet;
    struct icons icons;
    struct file_group group[FILE_GROUP_MAX];
    struct file files[FILE_MAX];
};

#define MAX_PATH_LEN 512
struct
file_browser
{
    /* path */
    char file[MAX_PATH_LEN];
    char home[MAX_PATH_LEN];
    char directory[MAX_PATH_LEN];

    /* directory content */
    char **files;
    char **directories;
    size_t file_count;
    size_t dir_count;
    struct media *media;
};

#if defined __unix__ || defined __APPLE__
#include <dirent.h>
#include <unistd.h>
#endif

#ifndef _WIN32
#include <pwd.h>
#endif

global_function
char*
StrDuplicate(const char *src)
{
    char *ret;
    size_t len = strlen(src);
    if (!len) return 0;
    ret = (char*)malloc(len+1);
    if (!ret) return 0;
    memcpy(ret, src, len);
    ret[len] = '\0';
    return ret;
}

global_function
void
DirFreeList(char **list, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        free(list[i]);
    free(list);
}

global_function
u32
StringIsLexBigger(char *string, char *toCompareTo)
{
    u32 result;
    u32 equal;

    do
    {
        equal = *string == *toCompareTo;
        result = *string > *(toCompareTo++);
    } while (equal && (*(string++) != '\0'));

    return(result);
}

global_function
void
CharArrayBubbleSort(char **list, u32 size)
{
    while (size > 1)
    {
        u32 newSize = 0;
        ForLoop(size - 1)
        {    
            if (StringIsLexBigger(list[index], list[index + 1]))
            {
                char *tmp = list[index];
                list[index] = list[index + 1];
                list[index + 1] = tmp;
                newSize = index + 1;
            }
        }
        size = newSize;
    }
}

global_function
char**
DirList(const char *dir, u32 return_subdirs, size_t *count)
{
    size_t n = 0;
    char buffer[MAX_PATH_LEN];
    char **results = NULL;
#ifndef _WIN32
    const DIR *none = NULL;
    DIR *z;
#else
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char dirBuff[MAX_PATH_LEN];
#endif
    size_t capacity = 32;
    size_t size;

    Assert(dir);
    Assert(count);
    strncpy(buffer, dir, MAX_PATH_LEN);
    n = strlen(buffer);

#ifndef _WIN32
    if (n > 0 && (buffer[n-1] != '/'))
        buffer[n++] = '/';
#else
    if (n > 0 && (buffer[n-1] != '\\'))
        buffer[n++] = '\\';
#endif

    size = 0;

#ifndef _WIN32
    z = opendir(dir);
#else
    strncpy(dirBuff, buffer, MAX_PATH_LEN);
    dirBuff[n] = '*';
    hFind = FindFirstFile(dirBuff, &ffd);
#endif
    
#ifndef _WIN32
    if (z != none)
#else
    if (hFind != INVALID_HANDLE_VALUE)
#endif
    {
#ifndef _WIN32
        u32 nonempty = 1;
        struct dirent *data = readdir(z);
        nonempty = (data != NULL);
        if (!nonempty) return NULL;
#endif
        do
        {
#ifndef _WIN32
            DIR *y;
#endif
            char *p;
            u32 is_subdir;
#ifndef _WIN32
            if (data->d_name[0] == '.')
#else
            if (ffd.cFileName[0] == '.') 
#endif
                continue;

#ifndef _WIN32
            strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);
            y = opendir(buffer);
            is_subdir = (y != NULL);
            if (y != NULL) closedir(y);
#else
            strncpy(buffer + n, ffd.cFileName, MAX_PATH_LEN-n);
            is_subdir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
#endif

            if ((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs))
            {
                if (!size)
                {
                    results = (char**)calloc(sizeof(char*), capacity);
                } 
                else if (size >= capacity)
                {
                    void *old = results;
                    capacity = capacity * 2;
                    results = (char**)realloc(results, capacity * sizeof(char*));
                    Assert(results);
                    if (!results) free(old);
                }
#ifndef _WIN32
                p = StrDuplicate(data->d_name);
#else
                p = StrDuplicate(ffd.cFileName);
#endif  
                results[size++] = p;
            }
#ifndef _WIN32    
        } while ((data = readdir(z)) != NULL);
#else
        } while (FindNextFile(hFind, &ffd) != 0);
#endif
    }

#ifndef _WIN32
    if (z) closedir(z);
#else
    FindClose(hFind);
#endif
    *count = size;
    
    CharArrayBubbleSort(results, (u32)size);
    
    return results;
}

global_function
struct file_group
FILE_GROUP(enum file_groups group, const char *name, struct nk_image *icon)
{
    struct file_group fg;
    fg.group = group;
    fg.name = name;
    fg.icon = icon;
    return fg;
}

global_function
struct file
FILE_DEF(enum file_types type, const char *suffix, enum file_groups group)
{
    struct file fd;
    fd.type = type;
    fd.suffix = suffix;
    fd.group = group;
    return fd;
}

global_function
struct nk_image*
MediaIconForFile(struct media *media, const char *file)
{
    u32 i = 0;
    const char *s = file;
    char suffix[16];
    u32 found = 0;
    memset(suffix, 0, sizeof(suffix));

    /* extract suffix .xxx from file */
    while (*s++ != '\0')
    {
        if (found && i < (sizeof(suffix)-1))
            suffix[i++] = *s;

        if (*s == '.')
        {
            if (found)
            {
                found = 0;
                break;
            }
            found = 1;
        }
    }

    /* check for all file definition of all groups for fitting suffix*/
    for (   i = 0;
            i < FILE_MAX && found;
            ++i)
    {
        struct file *d = &media->files[i];
        {
            const char *f = d->suffix;
            s = suffix;
            while (f && *f && *s && *s == *f)
            {
                s++; f++;
            }

            /* found correct file definition so */
            if (f && *s == '\0' && *f == '\0')
                return media->group[d->group].icon;
        }
    }
    return &media->icons.default_file;
}

global_function
void
MediaInit(struct media *media)
{
    /* file groups */
    struct icons *icons = &media->icons;
    media->group[FILE_GROUP_DEFAULT] = FILE_GROUP(FILE_GROUP_DEFAULT,"default",&icons->default_file);
    media->group[FILE_GROUP_PRETEXT] = FILE_GROUP(FILE_GROUP_PRETEXT, "pretext", &icons->img_file);

    /* files */
    media->files[FILE_DEFAULT] = FILE_DEF(FILE_DEFAULT, NULL, FILE_GROUP_DEFAULT);
    media->files[FILE_PRETEXT] = FILE_DEF(FILE_PRETEXT, "pretext", FILE_GROUP_PRETEXT);
    media->files[FILE_PSTM] = FILE_DEF(FILE_PSTM, "pstm", FILE_GROUP_PRETEXT);
}

global_function
void
FileBrowserReloadDirectoryContent(struct file_browser *browser, const char *path)
{
    strncpy(browser->directory, path, MAX_PATH_LEN);
    DirFreeList(browser->files, browser->file_count);
    DirFreeList(browser->directories, browser->dir_count);
    browser->files = DirList(path, 0, &browser->file_count);
    browser->directories = DirList(path, 1, &browser->dir_count);
}

global_function
void
FileBrowserInit(struct file_browser *browser, struct media *media)
{
    memset(browser, 0, sizeof(*browser));
    browser->media = media;
    {
        /* load files and sub-directory list */
        const char *home = getenv("HOME");
#ifdef _WIN32
        if (!home) home = getenv("USERPROFILE");
#else
        if (!home) home = getpwuid(getuid())->pw_dir;
#endif
        {
            size_t l;
            strncpy(browser->home, home, MAX_PATH_LEN);
            l = strlen(browser->home);
#ifdef _WIN32
      char *sep = (char *)"\\";
#else
      char *sep = (char *)"/";
#endif
            strcpy(browser->home + l, sep);
            strcpy(browser->directory, browser->home);
        }
        
        browser->files = DirList(browser->directory, 0, &browser->file_count);
        browser->directories = DirList(browser->directory, 1, &browser->dir_count);
    }
}

global_variable
char
Save_State_Name_Buffer[1024] = {0};

global_variable
char
AGP_Name_Buffer[1024] = {0};

global_function
void
SetSaveStateNameBuffer(char *name)
{
    u32 ptr = 0;
    while (*name) 
    {
        AGP_Name_Buffer[ptr] = *name;
        Save_State_Name_Buffer[ptr++] = *name++;
    }
    
    u32 ptr1 = ptr;
    name = (char *)".savestate_1";
    while (*name) Save_State_Name_Buffer[ptr++] = *name++;
    Save_State_Name_Buffer[ptr] = 0;

    ptr = ptr1;
    name = (char *)".agp_1";
    while (*name) AGP_Name_Buffer[ptr++] = *name++;
    AGP_Name_Buffer[ptr] = 0;
}

global_function
u08
FileBrowserRun(const char *name, struct file_browser *browser, struct nk_context *ctx, u32 show, u08 save = 0)
{
#ifndef _WIN32
    char pathSep = '/';
#else
    char pathSep = '\\';
#endif
   
    struct nk_window *window = nk_window_find(ctx, name);
    u32 doesExist = window != 0;

    if (!show && !doesExist)
    {
        return(0);
    }

    if (show && doesExist && (window->flags & NK_WINDOW_HIDDEN))
    {
        window->flags &= ~(nk_flags)NK_WINDOW_HIDDEN;
        FileBrowserReloadDirectoryContent(browser, browser->directory);
    }

    u08 ret = 0;
    struct media *media = browser->media;
    struct nk_rect total_space;

    if (nk_begin(ctx, name, nk_rect(Screen_Scale.x * 50, Screen_Scale.y * 50, Screen_Scale.x * 800, Screen_Scale.y * 700),
                NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE|NK_WINDOW_CLOSABLE))
    {
        static f32 ratio[] = {0.25f, NK_UNDEFINED};
        f32 spacing_x = ctx->style.window.spacing.x;
        nk_style_set_font(ctx, &NK_Font_Browser->handle);

        /* output path directory selector in the menubar */
        ctx->style.window.spacing.x = 0;
        nk_menubar_begin(ctx);
        {
            char *d = browser->directory;
            char *begin = d + 1;
            nk_layout_row_dynamic(ctx, Screen_Scale.y * 25.0f, 6);
            while (*d++)
            {
                if (*d == pathSep)
                {
                    *d = '\0';
                    if (nk_button_label(ctx, begin))
                    {
                        *d++ = pathSep; *d = '\0';
                        FileBrowserReloadDirectoryContent(browser, browser->directory);
                        break;
                    }
                    *d = pathSep;
                    begin = d + 1;
                }
            }
        }
        nk_menubar_end(ctx);
        ctx->style.window.spacing.x = spacing_x;

        /* window layout */
        f32 endSpace = save ? 100.0f : 0; // Resolved: end space is clipped as the space is not enough
        total_space = nk_window_get_content_region(ctx);
        nk_layout_row(ctx, NK_DYNAMIC, total_space.h - endSpace, 2, ratio);
        nk_group_begin(ctx, "Special", NK_WINDOW_NO_SCROLLBAR);
        {
            struct nk_image home = media->icons.home;
            struct nk_image computer = media->icons.computer;

            nk_layout_row_dynamic(ctx, Screen_Scale.y * 40.0f, 1);
            if (nk_button_image_label(ctx, home, "home", NK_TEXT_CENTERED))
                FileBrowserReloadDirectoryContent(browser, browser->home);
            if (nk_button_image_label(ctx,computer,"computer",NK_TEXT_CENTERED))
#ifndef _WIN32
                FileBrowserReloadDirectoryContent(browser, "/");
#else
                FileBrowserReloadDirectoryContent(browser, "C:\\");
#endif
            nk_group_end(ctx);
        }

        /* output directory content window */
        nk_group_begin(ctx, "Content", 0);
        {
            s32 index = -1;
            size_t i = 0, j = 0;//, k = 0;
            size_t rows = 0, cols = 0;
            size_t count = browser->dir_count + browser->file_count;
            f32 iconRatio[] = {0.05f, NK_UNDEFINED};

            cols = 1;
            rows = count / cols;
            for (   i = 0;
                    i <= rows;
                    i += 1)
            {
                {
                    size_t n = j + cols;
                    nk_layout_row(ctx, NK_DYNAMIC, Screen_Scale.y * 25.0f, 2, iconRatio);
                    for (   ; 
                            j < count && j < n;
                            ++j)
                    {
                        /* draw one row of icons */
                        if (j < browser->dir_count)
                        {
                            /* draw and execute directory buttons */
                            if (nk_button_image(ctx,media->icons.directory))
                                index = (s32)j;
                            nk_label(ctx, browser->directories[j], NK_TEXT_LEFT);
                        } 
                        else 
                        {
                            /* draw and execute files buttons */
                            struct nk_image *icon;
                            size_t fileIndex = ((size_t)j - browser->dir_count);
                            icon = MediaIconForFile(media,browser->files[fileIndex]);
                            if (nk_button_image(ctx, *icon))
                            {
                                if (save)
                                {
                                    strncpy(save == 2 ? AGP_Name_Buffer : Save_State_Name_Buffer, browser->files[fileIndex], sizeof(save == 2 ? AGP_Name_Buffer : Save_State_Name_Buffer));
                                }
                                else
                                {
                                    strncpy(browser->file, browser->directory, MAX_PATH_LEN);
                                    n = strlen(browser->file);
                                    strncpy(browser->file + n, browser->files[fileIndex], MAX_PATH_LEN - n);
                                    ret = 1;
                                }
                            }
                            nk_label(ctx,browser->files[fileIndex],NK_TEXT_LEFT);
                        }
                    }
                }
            }

            if (index != -1)
            {
                size_t n = strlen(browser->directory);
                strncpy(browser->directory + n, browser->directories[index], MAX_PATH_LEN - n);
                n = strlen(browser->directory);
                if (n < MAX_PATH_LEN - 1)
                {
                    browser->directory[n] = pathSep;
                    browser->directory[n+1] = '\0';
                }
                FileBrowserReloadDirectoryContent(browser, browser->directory);
            }
            
            nk_group_end(ctx);
        }

        if (save)
        {
            Deferred_Close_UI = 0;

            nk_layout_row(ctx, NK_DYNAMIC, endSpace - 5.0f, 1, ratio + 1);
            nk_group_begin(ctx, "File", NK_WINDOW_NO_SCROLLBAR);
            {
                f32 fileRatio[] = {0.8f, 0.1f, NK_UNDEFINED};
                f32 fileRatio2[] = {0.45f, 0.1f, 0.18f, 0.17f, NK_UNDEFINED};
                nk_layout_row(ctx, NK_DYNAMIC, Screen_Scale.y * 35.0f, save == 2 ? 5 : 3, save == 2 ? fileRatio2 : fileRatio);

                u08 saveViaEnter = (nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD | NK_EDIT_SIG_ENTER, save == 2 ? AGP_Name_Buffer : Save_State_Name_Buffer, sizeof(save == 2 ? AGP_Name_Buffer : Save_State_Name_Buffer), 0) & NK_EDIT_COMMITED) ? 1 : 0;
                
                static u08 overwrite = 0;
                overwrite = (u08)nk_check_label(ctx, "Override", overwrite);
                
                static u08 singletons = 0;
                if (save == 2) singletons = (u08)nk_check_label(ctx, "Format Singletons", singletons);
                static u08 preserveOrder = 0;
                if (save == 2) preserveOrder = (u08)nk_check_label(ctx, "Preserve Order", preserveOrder);

                if (nk_button_label(ctx, "Save") || saveViaEnter)
                {
                    strncpy(browser->file, browser->directory, MAX_PATH_LEN);
                    size_t n = strlen(browser->file);
                    strncpy(browser->file + n, save == 2 ? AGP_Name_Buffer : Save_State_Name_Buffer, MAX_PATH_LEN - n);
                    ret = 1 | (overwrite ? 2 : 0) | (singletons ? 4 : 0) | (preserveOrder ? 8 : 0);
                }

                nk_group_end(ctx);
            }
        }
        
        nk_style_set_font(ctx, &NK_Font->handle);
    }
    nk_end(ctx);
    
    return(ret);
}

global_function
void
MetaTagsEditorRun(struct nk_context *ctx, u08 show)
{
    const char *name = "Meta Data Tag Editor";
    struct nk_window *window = nk_window_find(ctx, name);

    u08 doesExist = window != 0;
    if (!show && !doesExist) return;
    if (show && doesExist && (window->flags & NK_WINDOW_HIDDEN)) window->flags &= ~(nk_flags)NK_WINDOW_HIDDEN;

    if (nk_begin(ctx, name, nk_rect(Screen_Scale.x * 50, Screen_Scale.y * 50, Screen_Scale.x * 800, Screen_Scale.y * 600),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_CLOSABLE|NK_WINDOW_NO_SCROLLBAR))
    {
        Deferred_Close_UI = 0;

        struct nk_rect total_space = nk_window_get_content_region(ctx);
        f32 ratio[] = {0.05f, NK_UNDEFINED};
        nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 1, ratio + 1);

        nk_group_begin(ctx, "Content", 0);
        {
            nk_layout_row(ctx, NK_DYNAMIC, Screen_Scale.y * 35.0f, 2, ratio);

            ForLoop(ArrayCount(Meta_Data->tags))
            {
                char buff[4];
                stbsp_snprintf(buff, sizeof(buff), "%u:", index + 1);
                nk_label(ctx, buff, NK_TEXT_LEFT);
                if (nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, (char *)Meta_Data->tags[index], sizeof(Meta_Data->tags[index]), 0) & NK_EDIT_ACTIVE)
                {
                    if (!strlen((const char *)Meta_Data->tags[index]))
                    {
                        ForLoop2(Number_of_Pixels_1D) Map_State->metaDataFlags[index2] &= ~(1 << index);
                        u32 nextActive = index;
                        ForLoop2((ArrayCount(Meta_Data->tags) - 1))
                        {
                            if (++nextActive == ArrayCount(Meta_Data->tags)) nextActive = 0;
                            if (strlen((const char *)Meta_Data->tags[nextActive]))
                            {
                                MetaData_Active_Tag = nextActive;
                                break;
                            }
                        }
                    }
                    else if (!strlen((const char *)Meta_Data->tags[MetaData_Active_Tag])) MetaData_Active_Tag = index;
                }
            }
            
            nk_group_end(ctx);
        }
    }
    nk_end(ctx);
}

global_function
void
AboutWindowRun(struct nk_context *ctx, u32 show)
{
    struct nk_window *window = nk_window_find(ctx, "About");
    u32 doesExist = window != 0;

    if (!show && !doesExist)
    {
        return;
    }

    if (show && doesExist && (window->flags & NK_WINDOW_HIDDEN))
    {
        window->flags &= ~(nk_flags)NK_WINDOW_HIDDEN;
    }

    enum windowMode {showAcknowledgements, showLicence, showThirdParty};

    static windowMode mode = showAcknowledgements;

    if (nk_begin_titled(ctx, "About", PretextView_Version, nk_rect(Screen_Scale.x * 50, Screen_Scale.y * 50, Screen_Scale.x * 870, Screen_Scale.y * 610),
                NK_WINDOW_BORDER|NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE|NK_WINDOW_CLOSABLE))
    {
        nk_menubar_begin(ctx);
        {
            nk_layout_row_dynamic(ctx, Screen_Scale.y * 35.0f, 3);
            if (nk_button_label(ctx, "Acknowledgements"))
            {
                mode = showAcknowledgements;
            }
            if (nk_button_label(ctx, "Licence"))
            {
                mode = showLicence;
            }
            if (nk_button_label(ctx, "Third Party Software"))
            {
                mode = showThirdParty;
            }
        }
        nk_menubar_end(ctx);

        struct nk_rect total_space = nk_window_get_content_region(ctx);
        f32 one = NK_UNDEFINED;
        nk_layout_row(ctx, NK_DYNAMIC, total_space.h, 1, &one);

        nk_group_begin(ctx, "About_Content", 0);
        {
            if (mode == showThirdParty)
            {
                u08 text[] = R"text(PretextView was made possible thanks to the following third party libraries and
resources, click each entry to view its licence.)text";

                nk_layout_row_static(ctx, Screen_Scale.y * 60, (s32)(Screen_Scale.x * 820), 1);
                s32 len = sizeof(text);
                nk_edit_string(ctx, NK_EDIT_READ_ONLY | NK_EDIT_NO_CURSOR | NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE, (char *)text, &len, len, 0);

                ForLoop(Number_of_ThirdParties)
                {
                    u32 nameIndex = 2 * index;
                    u32 licenceIndex = nameIndex + 1;
                    s32 *sizes = ThirdParty_Licence_Sizes[index];

                    if (nk_tree_push_id(NK_Context, NK_TREE_TAB, (const char *)ThirdParty[nameIndex], NK_MINIMIZED, (s32)index))
                    {
                        nk_layout_row_static(ctx, Screen_Scale.y * (f32)sizes[0], (s32)(Screen_Scale.x * (f32)sizes[1]), 1);
                        len = (s32)StringLength(ThirdParty[licenceIndex]);
                        nk_edit_string(ctx, NK_EDIT_READ_ONLY | NK_EDIT_NO_CURSOR | NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE, (char *)ThirdParty[licenceIndex], &len, len, 0);
                        nk_tree_pop(NK_Context);
                    }
                }
            }
            else if (mode == showAcknowledgements)
            {
                nk_layout_row_static(ctx, Screen_Scale.y * 500, (s32)(Screen_Scale.x * 820), 1);
                s32 len = sizeof(Acknowledgements);
                nk_edit_string(ctx, NK_EDIT_READ_ONLY | NK_EDIT_NO_CURSOR | NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE, (char *)Acknowledgements, &len, len, 0);
            }
            else
            {
                nk_layout_row_static(ctx, Screen_Scale.y * 500, (s32)(Screen_Scale.x * 820), 1);
                s32 len = sizeof(Licence);
                nk_edit_string(ctx, NK_EDIT_READ_ONLY | NK_EDIT_NO_CURSOR | NK_EDIT_SELECTABLE | NK_EDIT_MULTILINE, (char *)Licence, &len, len, 0);
            }
            
            nk_group_end(ctx);
        }
    }

    nk_end(ctx);
}



global_function void
UserSaveState(const char *, u08, char *);

global_function
void
default_user_profile_settings () ;

global_variable char
profile_savebuff[1024] = {0};

global_function
u08
UserProfileEditorRun(const char *name, struct file_browser *browser, struct nk_context *ctx, u32 show, u08 save = 0)
{
    struct nk_window *window = nk_window_find(ctx, name);
    u32 doesExist = window != 0;

    if (!show && !doesExist) // 不显示 且 不存在
    {
        return (0);
    }

    if (show && doesExist && (window->flags & NK_WINDOW_HIDDEN))  // 显示 且 存在 且 隐藏
    {
        window->flags &= ~(nk_flags)NK_WINDOW_HIDDEN;  // 取消隐藏
        FileBrowserReloadDirectoryContent(browser, browser->directory);
    }

    u08 ret = 0;
    struct media *media = browser->media;
    struct nk_rect total_space;

    if (nk_begin(ctx, name, nk_rect(Screen_Scale.x * 50, Screen_Scale.y * 50, Screen_Scale.x * 830, Screen_Scale.y * 600),
                 NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE | NK_WINDOW_SCALABLE))
    {

        nk_layout_row_dynamic(ctx, Screen_Scale.y * 30.0f, 1);

        if (nk_button_label(ctx, "Save") && window && !(window->flags & NK_WINDOW_HIDDEN))
        {
            ret = 2;
        }

        if (nk_button_label(ctx, "Default") && window && !(window->flags & NK_WINDOW_HIDDEN))
        {
            default_user_profile_settings(); 
            ret = 2;
        }

        // invert mouse setting
        nk_layout_row_dynamic(ctx, Screen_Scale.y * 30.0f, 4);
        bool original_value = user_profile_settings_ptr->invert_mouse;
        nk_checkbox_label(NK_Context, "Invert Mouse Buttons", (int*)&user_profile_settings_ptr->invert_mouse);
        if (original_value != user_profile_settings_ptr->invert_mouse)
        {   
            fmt::print("[UserPofile]: Mouse_invert settings changed ({}) -> ({})\n", original_value, user_profile_settings_ptr->invert_mouse);
            ret = 2;
        }
        
        // extensions
        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 4);

        if (Extensions.head)
        {
            // extensions
            if (nk_tree_push(NK_Context, NK_TREE_TAB, "Extensions", NK_MINIMIZED))

            {
                char buff[128];

                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);

                TraverseLinkedList(Extensions.head, extension_node)
                {
                    switch (node->type)
                    {
                    case extension_graph:
                    {
                        graph *gph = (graph *)node->extension;

                        stbsp_snprintf(buff, sizeof(buff), "Graph: %s", (char *)gph->name);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 5);
                        gph->on = nk_check_label(NK_Context, buff, (s32)gph->on) ? 1 : 0;

                        u08 currselection = gph->activecolor; // Start with the graph's current color selection

                        ForLoop(4)
                        {
                            // Update currselection based on user choice in nk_option_label
                            if (nk_option_label(NK_Context, colour_graph[index], currselection == index))
                            {
                                currselection = index;
                            }
                        }

                        if (currselection != gph->activecolor)
                        {
                            gph->activecolor = currselection;
                            gph->colour = graphColors[gph->activecolor];
                        }
                    }
                    break;
                    }
                }

                nk_tree_pop(NK_Context);
            }
        }

        // metadata tags
        if (nk_tree_push(NK_Context, NK_TREE_TAB, "MetaData Tag Settings", NK_MINIMIZED))
        {
            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3);

            u08 select = meta_outline->on;

            for (u32 i = 0; i < 3; i++)
            {
                if (nk_option_label(NK_Context, outlineColors[i], select == i))
                {
                    select = i;
                }
            }
            if (meta_outline->on != select)
            {
                meta_outline->on = select;
            }

            meta_outline->on = select;

            u08 selection = meta_data_curcolorProfile;
            ForLoop(4)
            {
                // Update currselection based on user choice in nk_option_label
                if (nk_option_label(NK_Context, metaColors[index], selection == index))
                {
                    selection = index;
                }
            }

            if (meta_data_curcolorProfile != selection)
            {
                meta_data_curcolorProfile = selection;
            }
            nk_tree_pop(NK_Context);
        }

        // Background colour
        if (nk_tree_push(NK_Context, NK_TREE_TAB, "Background Colour", NK_MINIMIZED))
        {
            char buff[128];
            u08 selection = active_bgcolor;

            ForLoop(7)
            {
                // Update currselection based on user choice in nk_option_label
                if (nk_option_label(NK_Context, bg_color[index], selection == index))
                {
                    selection = index;
                }
            }

            if (active_bgcolor != selection)
            {
                active_bgcolor = selection;
            }

            nk_tree_pop(NK_Context);
        }

        // Custom ordering for Colour maps
        if (nk_tree_push(NK_Context, NK_TREE_TAB, "Color Map Order", NK_MINIMIZED))
        {
            static s32 selectedIndex = -1; 
            bool orderChanged = false;

            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
            if (nk_checkbox_label(NK_Context, "Use Custom Color Map Order", &useCustomOrder))
            {
                orderChanged = true;
            }

            // "Reset to Default Order" button
            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
            if (nk_button_label(NK_Context, "Reset to Default Order"))
            {
                for (u32 i = 0; i < Color_Maps->nMaps; i++)
                {
                    userColourMapOrder.order[i] = i;
                }
                selectedIndex = -1;
                orderChanged = true;
            }

            if (useCustomOrder) // adjust the color map order
            {
                for (u32 i = 0; i < Color_Maps->nMaps; i++)
                {
                    u32 mapIndex = userColourMapOrder.order[i];

                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 50.0f, 1);

                    if (nk_group_begin(NK_Context, Color_Map_Names[mapIndex], NK_WINDOW_NO_SCROLLBAR))
                    {   
                        bool pushed_flag = false;
                        if (i == selectedIndex)
                        {
                            nk_style_push_color(NK_Context, &NK_Context->style.window.background, nk_rgb(150, 150, 200));
                            pushed_flag = true;
                        }

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                        if (nk_button_label(NK_Context, Color_Map_Names[mapIndex]))
                        {
                            if (selectedIndex == -1)
                            {
                                selectedIndex = i;
                            }
                            else
                            {
                                // Swap the selected items
                                u32 temp = userColourMapOrder.order[selectedIndex];
                                userColourMapOrder.order[selectedIndex] = userColourMapOrder.order[i];
                                userColourMapOrder.order[i] = temp;
                                selectedIndex = -1;
                                orderChanged = true;
                            }
                        }

                        if (pushed_flag) nk_style_pop_color(NK_Context);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 40.0f, 1);
                        nk_image(NK_Context, Color_Maps->mapPreviews[mapIndex]);

                        nk_group_end(NK_Context);
                    }
                }
            }
            else
            {
                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                nk_label(NK_Context, "Enable custom order to reorder color maps", NK_TEXT_LEFT);
            }

            nk_tree_pop(NK_Context);
        }


        // select the color for the selected area
        if (nk_tree_push(NK_Context, NK_TREE_TAB, "Colour Selected Area", NK_MINIMIZED))
        {
            struct nk_colorf colour_bg = {
                auto_curation_state.mask_color[0], 
                auto_curation_state.mask_color[1], 
                auto_curation_state.mask_color[2], 
                auto_curation_state.mask_color[3]};
            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200.0f, 4);
            colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);
            auto_curation_state.mask_color[0] = colour_bg.r;
            auto_curation_state.mask_color[1] = colour_bg.g;
            auto_curation_state.mask_color[2] = colour_bg.b;
            auto_curation_state.mask_color[3] = colour_bg.a;
            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 4);
            if (nk_button_label(NK_Context, "Default")) 
            {
                auto_curation_state.set_mask_color(auto_curation_state.mask_color_default);
            }
            nk_tree_pop(NK_Context);
        }
        // nk_tree_pop(NK_Context); // used to have this here, but it was causing a crash

        nk_style_set_font(ctx, &NK_Font->handle);
    }
    nk_end(ctx);

    return ret; 
}


global_function
struct nk_image
IconLoad(u08 *buffer, u32 bufferSize)
{
    s32 x,y,n;
    GLuint tex;
    u08 *data = stbi_load_from_memory((const u08*)buffer, (s32)bufferSize, &x, &y, &n, 0);
    if (!data)
    {
        fprintf(stderr, "Failed to load image\n");
        exit(1);
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return nk_image_id((s32)tex);
}

global_function
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
    #ifdef DEBUG
        (void)source;
        (void)id;
        (void)length;
        (void)userParam;
    #endif
    fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

// #ifdef DEBUG
// global_function
// void GLAPIENTRY
// MessageCallback( 
//     GLenum source,
//     GLenum type,
//     GLuint id,
//     GLenum severity,
//     GLsizei length,
//     const GLchar* message,
//     const void* userParam )
// {
//     fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
//             ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
//             type, severity, message );
    
//     // u32 BreakHere = 0;
//     // (void)BreakHere;
// }
// #endif

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

global_variable
u08 *
SaveState_Path = 0;

global_variable
u08 *
SaveState_Name = 0;

global_function
void
SetSaveStatePaths()
{
    if (SaveState_Path)
    {   
        printf("SaveState_Path already set: %s\n", SaveState_Path);
        return;
    }
    
    char buff_[64];
    char *buff = (char *)buff_;
#ifndef _WIN32
    char sep = '/';
    const char *path = getenv("XDG_CONFIG_HOME");
    char *folder;
    char *folder2;
    if (path)
    {
        folder = (char *)"PretextView";
        folder2 = 0;
    }
    else
    {
        path = getenv("HOME");
        if (!path) path = getpwuid(getuid())->pw_dir;
        folder = (char *)".config";
        folder2 = (char *)"PretextView";
    }

    if (path)
    {
        while ((*buff++ = *path++)) {}
        *(buff - 1) = sep;
        while ((*buff++ = *folder++)) {}

        mkdir((char *)buff_, 0700);
        struct stat st = {};

        u32 goodPath = 0;

        if (!stat((char *)buff_, &st))
        {
            if (folder2)
            {
                *(buff - 1) = sep;
                while ((*buff++ = *folder2++)) {}

                mkdir((char *)buff_, 0700);
                if (!stat((char *)buff_, &st))
                {
                    goodPath = 1;
                }
            }
            else
            {
                goodPath = 1;
            }
        } 
        
        if (goodPath)
        {
            u32 n = StringLength((u08 *)buff_);
            SaveState_Path = PushArray(Working_Set, u08, n + 18);
            CopyNullTerminatedString((u08 *)buff_, SaveState_Path);
            SaveState_Path[n] = (u08)sep;
            SaveState_Name = SaveState_Path + n + 1;
            SaveState_Path[n + 17] = 0;
        }
    }
#else
    PWSTR path = 0;
    char sep = '\\';
    HRESULT hres = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &path);
    if (SUCCEEDED(hres))
    {
        PWSTR pathPtr = path;
        while ((*buff++ = *pathPtr++)) {}
        *(buff - 1) = sep;
        char *folder = (char *)"PretextView";
        while ((*buff++ = *folder++)) {}

        if (CreateDirectory((char *)buff_, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
        {
            u32 n = StringLength((u08*)buff_);
            SaveState_Path = PushArray(Working_Set, u08, n + 18);
            CopyNullTerminatedString((u08*)buff_, SaveState_Path);
            SaveState_Path[n] = (u08)sep;
            SaveState_Name = SaveState_Path + n + 1;
            SaveState_Path[n + 17] = 0;
        }
    }

    if (path) CoTaskMemFree(path);
#endif
    
}

global_variable
u08
SaveState_Magic[5] = {'p', 't', 's', 'x', 2};

global_variable
u08
SaveState_Magic_Tail_Auto = 2;

global_variable
u08
SaveState_Magic_Tail_Manual = 3;

/* 
保存当前状态，
    headerHash: 用于生成文件名的哈希值
    path: 保存路径
    overwrite: 是否覆盖
    version: 版本号，用于控制生成文件的版本，兼容上一个版本
*/
global_function
u08
SaveState(
    u64 headerHash, char *path = 0, u08 overwrite = 0, 
    u32 version = 0
)
{
    if (!path)
    {
        static u08 flipFlop = 0;
        headerHash += (u64)flipFlop;
        flipFlop = (flipFlop + 1) & 1;

        if (!SaveState_Path)
        {
            SetSaveStatePaths();
        }
    }
    
    if (path || SaveState_Path)
    {
        if (!path)
        {
            ForLoop(16)
            {
                u08 x = (u08)((headerHash >> (4 * index)) & 0xF);
                u08 c = 'a' + x;
                *(SaveState_Name + index) = c;
            }
        }
        
        u32 nEdits = my_Min(Edits_Stack_Size, Map_Editor->nEdits);
        u32 nWayp = Waypoint_Editor->nWaypointsActive;

        u32 nGraphPlots = 0;
        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
                case extension_graph:
                    {
                        ++nGraphPlots;
                    }
                    break;
            }
        }
        
        // number of scaffs and metaFlags
        u32 nScaffs = 0;
        u32 nMetaFlags = 0;
        ForLoop(Contigs->numberOfContigs)
        {
            if ((Contigs->contigs_arr + index)->scaffId) ++nScaffs;
            if (*(Contigs->contigs_arr + index)->metaDataFlags) ++nMetaFlags;
        }
        // number of meta tags
        u08 nMetaTags = 0;
        u32 totalMetaTagSpace = 0;
        ForLoop(ArrayCount(Meta_Data->tags))
        {
            u32 strLen;
            if ((strLen = (u32)strlen((const char *)Meta_Data->tags[index])) > 0)
            {
                ++nMetaTags;
                totalMetaTagSpace += (strLen + 1);
            }
        }

        u32 nFileBytes = 352 + (13 * nWayp) + (12 * nEdits) + ((nEdits + 7) >> 3) + (32 * nGraphPlots) + (8 * nScaffs) + sizeof(meta_mode_data) + sizeof(MetaData_Active_Tag) + 4 + (12 * nMetaFlags) + 1 + nMetaTags + totalMetaTagSpace;
        u08 *fileContents = PushArrayP(Loading_Arena, u08, nFileBytes);
        u08 *fileWriter = fileContents;

        // settings
        {
            u08 settings = (u08)Current_Theme;
            *fileWriter++ = settings;
           
            settings = 0;
            settings |= Waypoints_Always_Visible ? (1 << 0) : 0;
            settings |= Contig_Name_Labels->on ? (1 << 1) : 0;
            settings |= Scale_Bars->on ? (1 << 2) : 0;
            settings |= Grid->on ? (1 << 3) : 0;
            settings |= Contig_Ids->on ? (1 << 4) : 0;
            settings |= Tool_Tip->on ? (1 << 5) : 0;
            settings |= user_profile_settings_ptr->invert_mouse ? (1 << 6) : 0;
            settings |= Scaffs_Always_Visible ? (1 << 7) : 0;

            *fileWriter++ = settings;
        }
        
        // colours
        {
            ForLoop(32)
            {
                *fileWriter++ = ((u08 *)Scaff_Mode_Data)[index];
            }

            ForLoop(64)
            {
                *fileWriter++ = ((u08 *)Waypoint_Mode_Data)[index];
            }

            ForLoop(80)
            {
                *fileWriter++ = ((u08 *)Edit_Mode_Colours)[index];
            }

            ForLoop(32)
            {
                *fileWriter++ = ((u08 *)Contig_Name_Labels)[index + 4];
            }

            ForLoop(32)
            {
                *fileWriter++ = ((u08 *)Scale_Bars)[index + 4];
            }

            ForLoop(16)
            {
                *fileWriter++ = ((u08 *)Grid)[index + 4];
            }

            ForLoop(32)
            {
                *fileWriter++ = ((u08 *)Tool_Tip)[index + 4];
            }
        }

        // sizes
        {
            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Scaff_Mode_Data)[index + 32];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Waypoint_Mode_Data)[index + 64];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Contig_Name_Labels)[index + 36];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Scale_Bars)[index + 36];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Grid)[index + 20];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Contig_Ids)[index + 20];
            }

            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)Tool_Tip)[index + 36];
            }
        }

        // colour map
        {
            u08 colourMap = (u08)Color_Maps->currMap;

            *fileWriter++ = colourMap;
        }

        // gamma
        {
            ForLoop(12)
            {
                *fileWriter++ = ((u08 *)Color_Maps->controlPoints)[index];
            }
        }

        // camera
        {
            ForLoop(12)
            {
                *fileWriter++ = ((u08 *)&Camera_Position)[index];
            }
        }

        // edits
        {
            ForLoop(4)
            {
                *fileWriter++ = ((u08 *)&nEdits)[index]; // write number of edits
            }

            u32 editStackPtr = Map_Editor->editStackPtr == nEdits ? 0 : Map_Editor->editStackPtr;
            u32 nContigFlags = (nEdits + 7) >> 3;
            u08 *contigFlags = fileWriter + (12 * nEdits);
            memset(contigFlags, 0, nContigFlags);

            ForLoop(nEdits)
            {
                if (editStackPtr == nEdits)
                {
                    editStackPtr = 0;
                }
                
                map_edit *edit = Map_Editor->edits + editStackPtr;
                u32 x = (u32)((s32)edit->finalPix1 - edit->delta);
                u32 y = (u32)((s32)edit->finalPix2 - edit->delta);
                s32 d = edit->delta;

                *fileWriter++ = ((u08 *)&x)[0];
                *fileWriter++ = ((u08 *)&x)[1];
                *fileWriter++ = ((u08 *)&x)[2];
                *fileWriter++ = ((u08 *)&x)[3];
                *fileWriter++ = ((u08 *)&y)[0];
                *fileWriter++ = ((u08 *)&y)[1];
                *fileWriter++ = ((u08 *)&y)[2];
                *fileWriter++ = ((u08 *)&y)[3];
                *fileWriter++ = ((u08 *)&d)[0];
                *fileWriter++ = ((u08 *)&d)[1];
                *fileWriter++ = ((u08 *)&d)[2];
                *fileWriter++ = ((u08 *)&d)[3];

                if (edit->finalPix1 > edit->finalPix2)
                {
                    u32 byte = (index + 1) >> 3;
                    u32 bit = (index + 1) & 7;

                    contigFlags[byte] |= (1 << bit);
                }

                ++editStackPtr;
            }

            fileWriter += nContigFlags;
        }

        // waypoints
        {   
            u32 bytes_per_waypoint = 13;  // used to be 13
            *fileWriter++ = (u08)nWayp;

            u32 ptr = 348 + (bytes_per_waypoint * nWayp) + (12 * nEdits) + ((nEdits + 7) >> 3);
            TraverseLinkedList(Waypoint_Editor->activeWaypoints.next, waypoint)
            {
                f32 x = node->coords.x;
                f32 y = node->coords.y;
                f32 z = node->z;
                u08 id = (u08)node->index;

                fileContents[--ptr] = id;
                fileContents[--ptr] = ((u08 *)&z)[3];
                fileContents[--ptr] = ((u08 *)&z)[2];
                fileContents[--ptr] = ((u08 *)&z)[1];
                fileContents[--ptr] = ((u08 *)&z)[0];
                fileContents[--ptr] = ((u08 *)&y)[3];
                fileContents[--ptr] = ((u08 *)&y)[2];
                fileContents[--ptr] = ((u08 *)&y)[1];
                fileContents[--ptr] = ((u08 *)&y)[0];
                fileContents[--ptr] = ((u08 *)&x)[3];
                fileContents[--ptr] = ((u08 *)&x)[2];
                fileContents[--ptr] = ((u08 *)&x)[1];
                fileContents[--ptr] = ((u08 *)&x)[0];

                // if (bytes_per_waypoint == 15){  // better not to change the datasaved
                //     u16 long_waypoint_mode = (u16) node->long_waypoint_mode;
                //     fileContents[--ptr] = ((u08 *)&long_waypoint_mode)[1];
                //     fileContents[--ptr] = ((u08 *)&long_waypoint_mode)[0];
                // }
            }

            fileWriter += (bytes_per_waypoint * nWayp);
        }

        // scaffs
        {
            *fileWriter++ = ((u08 *)&nScaffs)[0];
            *fileWriter++ = ((u08 *)&nScaffs)[1];
            *fileWriter++ = ((u08 *)&nScaffs)[2];
            *fileWriter++ = ((u08 *)&nScaffs)[3];
            ForLoop(Contigs->numberOfContigs)
            {
                if ((Contigs->contigs_arr + index)->scaffId)
                {
                    u32 sId = (Contigs->contigs_arr + index)->scaffId;
                    *fileWriter++ = ((u08 *)&index)[0];
                    *fileWriter++ = ((u08 *)&index)[1];
                    *fileWriter++ = ((u08 *)&index)[2];
                    *fileWriter++ = ((u08 *)&index)[3];
                    *fileWriter++ = ((u08 *)&sId)[0];
                    *fileWriter++ = ((u08 *)&sId)[1];
                    *fileWriter++ = ((u08 *)&sId)[2];
                    *fileWriter++ = ((u08 *)&sId)[3];
                }
            }
        }

        // meta data
        {
            memcpy(fileWriter, (const void *)&MetaData_Active_Tag, sizeof(MetaData_Active_Tag));
            fileWriter += sizeof(MetaData_Active_Tag);
            
            memcpy(fileWriter, (const void *)MetaData_Mode_Data, sizeof(meta_mode_data));
            fileWriter += sizeof(meta_mode_data);

            *fileWriter++ = ((u08 *)&nMetaFlags)[0];
            *fileWriter++ = ((u08 *)&nMetaFlags)[1];
            *fileWriter++ = ((u08 *)&nMetaFlags)[2];
            *fileWriter++ = ((u08 *)&nMetaFlags)[3];
            ForLoop(Contigs->numberOfContigs)
            {
                if (*(Contigs->contigs_arr + index)->metaDataFlags)
                {
                    u64 flags = *(Contigs->contigs_arr + index)->metaDataFlags;
                    *fileWriter++ = ((u08 *)&index)[0];
                    *fileWriter++ = ((u08 *)&index)[1];
                    *fileWriter++ = ((u08 *)&index)[2];
                    *fileWriter++ = ((u08 *)&index)[3];
                    *fileWriter++ = ((u08 *)&flags)[0];
                    *fileWriter++ = ((u08 *)&flags)[1];
                    *fileWriter++ = ((u08 *)&flags)[2];
                    *fileWriter++ = ((u08 *)&flags)[3];
                    *fileWriter++ = ((u08 *)&flags)[4];
                    *fileWriter++ = ((u08 *)&flags)[5];
                    *fileWriter++ = ((u08 *)&flags)[6];
                    *fileWriter++ = ((u08 *)&flags)[7];
                }
            }

            *fileWriter++ = nMetaTags;
            ForLoop(ArrayCount(Meta_Data->tags))
            {
                u32 strLen;
                if ((strLen = (u32)strlen((const char *)Meta_Data->tags[index])) > 0)
                {
                    *fileWriter++ = (u08)index;
                    memcpy(fileWriter, (const void *)Meta_Data->tags[index], strLen + 1);
                    fileWriter += (strLen + 1);
                }
            }
        }

        // extensions
        {
            TraverseLinkedList(Extensions.head, extension_node)
            {
                switch (node->type)
                {
                    case extension_graph:
                        {
                            graph *gph = (graph *)node->extension;
                            ForLoop(32)
                            {
                                *fileWriter++ = ((u08 *)gph)[index + 88];
                            }
                        }
                        break;
                }
            }
        }
    
        u32 nBytesComp = nFileBytes + 128;
        u08 *compBuff = PushArrayP(Loading_Arena, u08, nBytesComp);
        u32 nCommpressedBytes = 0;

        if (!(nCommpressedBytes = (u32)libdeflate_deflate_compress(Compressor, (const void *)fileContents, nFileBytes, (void *)compBuff, nBytesComp)))
        {
            fprintf(stderr, "Could not compress save state file info the given buffer\n");
            exit(1);
        }

        FILE *file;
        if (path) // write to specific path
        {
            if (!overwrite)
            {
                if ((file = fopen((const char *)path, "rb")))
                {
                    fclose(file);
                    return(1);
                }
            }

            if (!(file = fopen((const char *)path, "wb"))) return(1);

            fwrite(SaveState_Magic, 1, sizeof(SaveState_Magic), file);
            fwrite(&SaveState_Magic_Tail_Manual, 1, 1, file);
            fwrite(&headerHash, 1, 8, file);
        }
        else // write to auto path
        {
            file = fopen((const char *)SaveState_Path, "wb");
            fwrite(SaveState_Magic, 1, sizeof(SaveState_Magic), file);
            fwrite(&SaveState_Magic_Tail_Auto, 1, 1, file);
        }
        
        fwrite(&nCommpressedBytes, 1, 4, file); // size after  compression
        fwrite(&nFileBytes, 1, 4, file);        // size before compression
        fwrite(compBuff, 1, nCommpressedBytes, file);
        fclose(file);

        FreeLastPushP(Loading_Arena); // compBuff
        FreeLastPushP(Loading_Arena); // fileContents

        if (!path) // save the name SaveState_Name to file named as ptlsn 
        {
            // u08 nameCache[17];
            // CopyNullTerminatedString((u08 *)SaveState_Name, (u08 *)nameCache);

            std::string nameCache_string = std::string((char *)SaveState_Name);

            *(SaveState_Name + 0) = 'p';
            *(SaveState_Name + 1) = 't';
            *(SaveState_Name + 2) = 'l';
            *(SaveState_Name + 3) = 's';
            *(SaveState_Name + 4) = 'n';
            *(SaveState_Name + 5) = '\0';

            file = fopen((const char *)SaveState_Path, "wb");
            // fwrite((u08 *)nameCache, 1, sizeof(nameCache), file);
            fwrite((u08 *)nameCache_string.c_str(), 1, nameCache_string.size(), file);
            fclose(file);
        }
    }

    return(0);
}

global_function
u08
LoadState(u64 headerHash, char *path)
{
    if (!path && !SaveState_Path)
    {
        SetSaveStatePaths(); // set the SaveState_Path, and assign the pointer to SaveState_Name
    }
    
    if (path || SaveState_Path)
    {
        FILE *file = 0;
        u08 fullLoad = 1;
        
        if (path) // load state file with a specific path
        {
            if ((file = fopen((const char *)path, "rb")))
            {
                u08 magicTest[sizeof(SaveState_Magic) + 1];

                u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file);
                if (bytesRead == sizeof(magicTest))
                {
                    ForLoop(sizeof(SaveState_Magic))
                    {
                        if (SaveState_Magic[index] != magicTest[index])
                        {
                            fclose(file);
                            file = 0;
                            break;
                        }
                    }
                    if (file)
                    {
                        if (magicTest[sizeof(magicTest) - 1] != SaveState_Magic_Tail_Manual)
                        {
                            fclose(file);
                            file = 0;
                        }
                        else
                        {
                            u64 hashTest;
                            bytesRead = (u32)fread(&hashTest, 1, sizeof(hashTest), file);
                            if (!(bytesRead == sizeof(hashTest) && hashTest == headerHash))
                            {
                                fclose(file);
                                file = 0;
                            }
                        }
                    }
                }
                else
                {
                    fclose(file);
                    file = 0;
                }
            }
        }
        else // load state file with the auto path
        {
            ForLoop(16)
            {
                u08 x = (u08)((headerHash >> (4 * index)) & 0xF);
                u08 c = 'a' + x;
                *(SaveState_Name + index) = c;
            }

            printf("SaveState_Name: %s\n", SaveState_Name);
            printf("SaveState_Path: %s\n", SaveState_Path);

            if ((file = fopen((const char *)SaveState_Path, "rb")))
            {
                u08 magicTest[sizeof(SaveState_Magic) + 1];

                u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file);
                if (bytesRead == sizeof(magicTest))
                {
                    ForLoop(sizeof(SaveState_Magic))
                    {
                        if (SaveState_Magic[index] != magicTest[index])
                        {
                            fclose(file);
                            file = 0;
                            break;
                        }
                    }
                    if (file)
                    {
                        if (magicTest[sizeof(magicTest) - 1] != SaveState_Magic_Tail_Auto)
                        {
                            fclose(file);
                            file = 0;
                        }
                    }
                }
                else
                {
                    fclose(file);
                    file = 0;
                }
            }
            else
            {
                fullLoad = 0;
                u08 nameCache[16];
                *(SaveState_Name + 0) = 'p';
                *(SaveState_Name + 1) = 't';
                *(SaveState_Name + 2) = 'l';
                *(SaveState_Name + 3) = 's';
                *(SaveState_Name + 4) = 'n';
                *(SaveState_Name + 5) = '\0';

                if ((file = fopen((const char *)SaveState_Path, "rb")))
                {
                    if (fread((u08 *)nameCache, 1, sizeof(nameCache), file) == sizeof(nameCache))
                    {
                        fclose(file);
                        file = 0;
                        ForLoop(16)
                        {
                            *(SaveState_Name + index) = nameCache[index];
                        }
                        if ((file = fopen((const char *)SaveState_Path, "rb")))
                        {
                            u08 magicTest[sizeof(SaveState_Magic) + 1];

                            u32 bytesRead = (u32)fread(magicTest, 1, sizeof(magicTest), file);
                            if (bytesRead == sizeof(magicTest))
                            {
                                ForLoop(sizeof(SaveState_Magic))
                                {
                                    if (SaveState_Magic[index] != magicTest[index])
                                    {
                                        fclose(file);
                                        file = 0;
                                        break;
                                    }
                                }
                                if (file)
                                {
                                    if (magicTest[sizeof(magicTest) - 1] != SaveState_Magic_Tail_Auto)
                                    {
                                        fclose(file);
                                        file = 0;
                                    }
                                }
                            }
                            else
                            {
                                fclose(file);
                                file = 0;
                            }
                        }

                    }
                    else
                    {
                        fclose(file);
                        file = 0;
                    }
                }
            }
        }
        
        if (file)
        {
            u32 nBytesComp;
            u32 nBytesFile;
            fread(&nBytesComp, 1, 4, file);
            fread(&nBytesFile, 1, 4, file);
            u32 nBytesRead = 0;

            u08 *fileContents = PushArrayP(Loading_Arena, u08, nBytesFile);
            u08 *compBuffer = PushArrayP(Loading_Arena, u08, nBytesComp);

            fread(compBuffer, 1, nBytesComp, file);
            fclose(file);
            if (libdeflate_deflate_decompress(Decompressor, (const void *)compBuffer, nBytesComp, (void *)fileContents, nBytesFile, NULL))
            {
                FreeLastPushP(Loading_Arena); // comp buffer
                FreeLastPushP(Loading_Arena); // fileContents
                return(1);
            }
            FreeLastPushP(Loading_Arena); // comp buffer

            // settings
            {
                u08 settings = *fileContents++;
                theme th = (theme)settings;
                SetTheme(NK_Context, th);

                settings = *fileContents++;
                Waypoints_Always_Visible = settings & (1 << 0);
                Contig_Name_Labels->on = settings & (1 << 1);
                Scale_Bars->on = settings & (1 << 2);
                Grid->on = settings & (1 << 3);
                Contig_Ids->on = settings & (1 << 4);
                Tool_Tip->on = settings & (1 << 5);
                // user_profile_settings_ptr->invert_mouse = settings & (1 << 6); // this is moved to the user settings, then this setting will not be influnced by clearcache
                Scaffs_Always_Visible = settings & (1 << 7);

                nBytesRead += 2;
            }

            // colours
            {
                ForLoop(32)
                {
                    ((u08 *)Scaff_Mode_Data)[index] = *fileContents++;
                }

                ForLoop(64)
                {
                    ((u08 *)Waypoint_Mode_Data)[index] = *fileContents++;
                }

                ForLoop(80)
                {
                    ((u08 *)Edit_Mode_Colours)[index] = *fileContents++;
                }

                ForLoop(32)
                {
                    ((u08 *)Contig_Name_Labels)[index + 4] = *fileContents++;
                }

                ForLoop(32)
                {
                    ((u08 *)Scale_Bars)[index + 4] = *fileContents++;
                }

                ForLoop(16)
                {
                    ((u08 *)Grid)[index + 4] = *fileContents++;
                }

                ForLoop(32)
                {
                    ((u08 *)Tool_Tip)[index + 4] = *fileContents++;
                }

                nBytesRead += 288;
            }

            // sizes
            {
                ForLoop(4)
                {
                    ((u08 *)Scaff_Mode_Data)[index + 32] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Waypoint_Mode_Data)[index + 64] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Contig_Name_Labels)[index + 36] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Scale_Bars)[index + 36] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Grid)[index + 20] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Contig_Ids)[index + 20] = *fileContents++;
                }

                ForLoop(4)
                {
                    ((u08 *)Tool_Tip)[index + 36] = *fileContents++;
                }

                nBytesRead += 28;
            }

            // colour map
            {
                u08 colourMap = *fileContents++;

                Color_Maps->currMap = (u32)colourMap;
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->currMap]);
                glActiveTexture(GL_TEXTURE0);

                ++nBytesRead;
            }

            // gamma
            {
                ForLoop(12)
                {
                    ((u08 *)Color_Maps->controlPoints)[index] = *fileContents++;
                }
                nBytesRead += 12;

                glUseProgram(Contact_Matrix->shaderProgram);
                glUniform3fv(Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
            }

            if (fullLoad)
            {
                // camera
                {
                    ForLoop(12)
                    {
                        ((u08 *)&Camera_Position)[index] = *fileContents++;
                    }
                    nBytesRead += 12;
                }

                // edits
                {
                    u32 nEdits  = my_Min(Edits_Stack_Size, Map_Editor->nEdits);
                    ForLoop(nEdits) UndoMapEdit();

                    ForLoop(4) ((u08 *)&nEdits)[index] = *fileContents++;
                    nBytesRead += 4;

                    u08 *contigFlags = fileContents + (12 * nEdits);
                    u32 nContigFlags = ((u32)nEdits + 7) >> 3;

                    ForLoop(nEdits)
                    {
                        u32 x;
                        u32 y;
                        s32 d;

                        ((u08 *)&x)[0] = *fileContents++;
                        ((u08 *)&x)[1] = *fileContents++;
                        ((u08 *)&x)[2] = *fileContents++;
                        ((u08 *)&x)[3] = *fileContents++;
                        ((u08 *)&y)[0] = *fileContents++;
                        ((u08 *)&y)[1] = *fileContents++;
                        ((u08 *)&y)[2] = *fileContents++;
                        ((u08 *)&y)[3] = *fileContents++;
                        ((u08 *)&d)[0] = *fileContents++;
                        ((u08 *)&d)[1] = *fileContents++;
                        ((u08 *)&d)[2] = *fileContents++;
                        ((u08 *)&d)[3] = *fileContents++;

                        u32 byte = (index + 1) >> 3; // find the byte, 1 byte saves 8 invert_flag of edits
                        u32 bit = (index + 1) & 7; // find the bit in the byte
                        u32 invert  = contigFlags[byte] & (1 << bit);

                        pointui startPixels = {x, y};
                        s32 delta = d;
                        pointui finalPixels = {(u32)((s32)startPixels.x + delta), (u32)((s32)startPixels.y + delta)};

                        RearrangeMap(startPixels.x, startPixels.y, delta);
                        if (invert) InvertMap(finalPixels.x, finalPixels.y);

                        AddMapEdit(delta, finalPixels, invert);
                    }

                    fileContents += nContigFlags;
                    nBytesRead += (nContigFlags + (12 * nEdits));
                }

                // waypoints
                {   
                    u32 bytes_per_waypoint = 13;  // used to be 13
                    TraverseLinkedList(Waypoint_Editor->activeWaypoints.next, waypoint)
                    {
                        waypoint *tmp = node->prev;
                        RemoveWayPoint(node);
                        node = tmp;
                    }
                    u08 nWayp = *fileContents++;
                    ++nBytesRead;

                    ForLoop(nWayp)
                    {
                        f32 x;
                        f32 y;
                        f32 z;
                        
                        // it is better not change the saved data structure
                        // u16 long_waypoint_mode;
                        // if (bytes_per_waypoint == 15){
                        //     ((u08 *)&long_waypoint_mode)[0] = *fileContents++;
                        //     ((u08 *)&long_waypoint_mode)[1] = *fileContents++;
                        // }

                        ((u08 *)&x)[0] = *fileContents++;
                        ((u08 *)&x)[1] = *fileContents++;
                        ((u08 *)&x)[2] = *fileContents++;
                        ((u08 *)&x)[3] = *fileContents++;
                        ((u08 *)&y)[0] = *fileContents++;
                        ((u08 *)&y)[1] = *fileContents++;
                        ((u08 *)&y)[2] = *fileContents++;
                        ((u08 *)&y)[3] = *fileContents++;
                        ((u08 *)&z)[0] = *fileContents++;
                        ((u08 *)&z)[1] = *fileContents++;
                        ((u08 *)&z)[2] = *fileContents++;
                        ((u08 *)&z)[3] = *fileContents++;
                        u08 id = *fileContents++;

                        AddWayPoint({x, y});
                        Waypoint_Editor->activeWaypoints.next->z = z;
                        Waypoint_Editor->activeWaypoints.next->index = (u32)id;
                        // if (bytes_per_waypoint == 15) { // better not change the data structure
                        //     Waypoint_Editor->activeWaypoints.next->long_waypoint_mode = (u16)long_waypoint_mode;
                        // }
                    }

                    nBytesRead += (bytes_per_waypoint * nWayp);
                }

                // scaffs
                {
                    u32 nScaffs;
                    ((u08 *)&nScaffs)[0] = *fileContents++;
                    ((u08 *)&nScaffs)[1] = *fileContents++;
                    ((u08 *)&nScaffs)[2] = *fileContents++;
                    ((u08 *)&nScaffs)[3] = *fileContents++;

                    nBytesRead += 4;

                    ForLoop(Contigs->numberOfContigs) (Contigs->contigs_arr + index)->scaffId = 0;

                    ForLoop(nScaffs)
                    {
                        u32 cId;
                        u32 sId;
                        ((u08 *)&cId)[0] = *fileContents++;
                        ((u08 *)&cId)[1] = *fileContents++;
                        ((u08 *)&cId)[2] = *fileContents++;
                        ((u08 *)&cId)[3] = *fileContents++;
                        ((u08 *)&sId)[0] = *fileContents++;
                        ((u08 *)&sId)[1] = *fileContents++;
                        ((u08 *)&sId)[2] = *fileContents++;
                        ((u08 *)&sId)[3] = *fileContents++;

                        (Contigs->contigs_arr + cId)->scaffId = sId;
                    }

                    UpdateScaffolds();

                    nBytesRead += (8 * nScaffs);
                }

                // meta data
                {
                    memcpy(&MetaData_Active_Tag, (const void *)fileContents, sizeof(MetaData_Active_Tag));
                    fileContents += sizeof(MetaData_Active_Tag);
                    nBytesRead += sizeof(MetaData_Active_Tag);

                    memcpy(MetaData_Mode_Data, (const void *)fileContents, sizeof(meta_mode_data));
                    fileContents += sizeof(meta_mode_data);
                    nBytesRead += sizeof(meta_mode_data);

                    u32 nMetaFlags;
                    ((u08 *)&nMetaFlags)[0] = *fileContents++;
                    ((u08 *)&nMetaFlags)[1] = *fileContents++;
                    ((u08 *)&nMetaFlags)[2] = *fileContents++;
                    ((u08 *)&nMetaFlags)[3] = *fileContents++;
                    nBytesRead += 4;

                    memset(Map_State->metaDataFlags, 0, Number_of_Pixels_1D * sizeof(u64));
                    ForLoop(nMetaFlags)
                    {
                        u32 cId;
                        u64 flags;
                        ((u08 *)&cId)[0] = *fileContents++;
                        ((u08 *)&cId)[1] = *fileContents++;
                        ((u08 *)&cId)[2] = *fileContents++;
                        ((u08 *)&cId)[3] = *fileContents++;
                        ((u08 *)&flags)[0] = *fileContents++;
                        ((u08 *)&flags)[1] = *fileContents++;
                        ((u08 *)&flags)[2] = *fileContents++;
                        ((u08 *)&flags)[3] = *fileContents++;
                        ((u08 *)&flags)[4] = *fileContents++;
                        ((u08 *)&flags)[5] = *fileContents++;
                        ((u08 *)&flags)[6] = *fileContents++;
                        ((u08 *)&flags)[7] = *fileContents++;
                        nBytesRead += 12;

                        u32 pixel = 0;
                        while ((pixel < Number_of_Pixels_1D) && (Map_State->contigIds[pixel] != cId)) ++pixel;
                        while ((pixel < Number_of_Pixels_1D) && (Map_State->contigIds[pixel] == cId)) Map_State->metaDataFlags[pixel++] = flags;
                    }
                    UpdateContigsFromMapState();

                    u08 nMetaTags = *fileContents++;
                    ++nBytesRead;

                    memset(Meta_Data, 0, sizeof(meta_data));
                    ForLoop(nMetaTags)
                    {
                        u08 id = *fileContents++;
                        strcpy((char *)Meta_Data->tags[id], (const char *)fileContents);
                        s32 strLenPlusOne = strlen((const char *)Meta_Data->tags[id]) + 1;
                        fileContents += strLenPlusOne;
                        nBytesRead += (strLenPlusOne + 1);
                    }
                }

                // extensions
                {
                    TraverseLinkedList(Extensions.head, extension_node)
                    {
                        switch (node->type)
                        {
                            case extension_graph:
                                {
                                    if ((nBytesRead + 32) > nBytesFile) break;

                                    ForLoop(32)
                                    {
                                        ((u08 *)node->extension)[index + 88] = *fileContents++;
                                    }

                                    nBytesRead += 32;
                                }
                                break;
                        }
                    }
                }
            }

            Redisplay = 1;
            FreeLastPushP(Loading_Arena); // fileContents
        }
    }

    return(0);
}


// restore the state before curation
global_function
void
restore_initial_state()
{
    u32 nBytesRead = 0;

    // settings
    {
        theme th = (theme) 4;
        SetTheme(NK_Context, th);

        Waypoints_Always_Visible = 1;
        Contig_Name_Labels->on   = 1;
        Scale_Bars->on           = 1;
        Grid->on                 = 1;
        Contig_Ids->on           = 1;
        Tool_Tip->on             = 1;
        // user_profile_settings_ptr   = 0;
        Scaffs_Always_Visible    = 1;
        MetaData_Always_Visible  = 1;

    }

    // colours && size
    {   
        {
            Scaff_Mode_Data->text = Yellow_Text_Float;
            Scaff_Mode_Data->bg = Grey_Background;
            Scaff_Mode_Data->size = DefaultScaffSize;
        }

        {
            Waypoint_Mode_Data->base = Red_Full;
            Waypoint_Mode_Data->selected = Blue_Full;
            Waypoint_Mode_Data->text = Yellow_Text_Float;
            Waypoint_Mode_Data->bg = Grey_Background;
            Waypoint_Mode_Data->size = DefaultWaypointSize;
        }

        {
            Edit_Mode_Colours->preSelect = Green_Float;
            Edit_Mode_Colours->select = Blue_Float;
            Edit_Mode_Colours->invSelect = Red_Float;
            Edit_Mode_Colours->fg = Yellow_Text_Float;
            Edit_Mode_Colours->bg = Grey_Background;
        }

        {
            Contig_Name_Labels->on = 0;
            Contig_Name_Labels->fg = Yellow_Text_Float;
            Contig_Name_Labels->bg = Grey_Background;
            Contig_Name_Labels->size = DefaultNameLabelTextSize;
        }

        {
            Scale_Bars->on = 0;
            Scale_Bars->fg = Red_Text_Float;
            Scale_Bars->bg = Grey_Background;
            Scale_Bars->size = DefaultScaleBarSize;
        }

        {
            Grid->on = 1;
            Grid->bg = Grey_Background;
            Grid->size = DefaultGridSize;
        }

        {
            Tool_Tip->on = 1;
            Tool_Tip->fg = Yellow_Text_Float;
            Tool_Tip->bg = Grey_Background;
            Tool_Tip->size = DefaultToolTipTextSize;
        }

        {
            Contig_Ids->on = 1;
            Contig_Ids->size = DefaultContigIdSize;
        }
    }

    // colour map
    {   // set as the first colour map
        Color_Maps->currMap = useCustomOrder ? userColourMapOrder.order[0] : 0;
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->currMap]);
        glActiveTexture(GL_TEXTURE0);
    }

    // gamma
    {
        Color_Maps->controlPoints[0] = 0.0f;
        Color_Maps->controlPoints[1] = 0.5f;
        Color_Maps->controlPoints[2] = 1.0f;

        glUseProgram(Contact_Matrix->shaderProgram);
        glUniform3fv(Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
    }

    // camera
    {
        Camera_Position.x = 0.0f;
        Camera_Position.y = 0.0f;
        Camera_Position.z = 1.0f;
    }

    // edits
    {
        u32 nEdits  = my_Min(Edits_Stack_Size, Map_Editor->nEdits);
        ForLoop(nEdits) UndoMapEdit();
    }
    
    // restore all the splited contigs
    {
        Map_State->restore_cutted_contigs_all(Number_of_Pixels_1D);
    }


    // waypoints
    {   
        TraverseLinkedList(Waypoint_Editor->activeWaypoints.next, waypoint)
        {
            waypoint *tmp = node->prev;
            RemoveWayPoint(node);
            node = tmp;
        }
    }

    // scaffs
    {
        ForLoop(Contigs->numberOfContigs) (Contigs->contigs_arr + index)->scaffId = 0;
        UpdateScaffolds();
    }

    // meta data
    {
        MetaData_Active_Tag = 0;

        MetaData_Mode_Data->text = Yellow_Text_Float;
        MetaData_Mode_Data->bg = Grey_Background;
        MetaData_Mode_Data->size = DefaultMetaDataSize;

        memset(Map_State->metaDataFlags, 0, Number_of_Pixels_1D * sizeof(u64));
        
        UpdateContigsFromMapState();

        for (u32 i = 0; i < ArrayCount(Meta_Data->tags); i ++ ) Meta_Data->tags[i][0] = 0;
        ForLoop(ArrayCount(Default_Tags)) strcpy((char *)Meta_Data->tags[MetaData_Active_Tag + index], Default_Tags[index]);
    }

    // extensions
    {
        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
                case extension_graph:
                    {
                        ((graph *)node->extension)->on = 0;
                        ((graph *)node->extension)->nameOn = 0;
                    }
                    break;
            }
        }
    }
    

    Redisplay = 1;

}



// User Profile
global_variable u08 *
    UserSaveState_Path = 0;

global_variable u08 *
    UserSaveState_Name = 0;

global_function void
UserSetSaveStatePaths()
{
    if (!UserSaveState_Path)
    {
        char buff_[64];
        char *buff = (char *)buff_;
#ifndef _WIN32
        char sep = '/';
        const char *path = getenv("XDG_CONFIG_HOME");
        char *folder;
        char *folder2;
        if (path)
        {
            folder = (char *)"PretextView";
            folder2 = 0;
        }
        else
        {
            path = getenv("HOME");
            if (!path)
                path = getpwuid(getuid())->pw_dir;
            folder = (char *)".config";
            folder2 = (char *)"PretextView";
        }

        if (path)
        {
            while ((*buff++ = *path++))
            {
            }
            *(buff - 1) = sep;
            while ((*buff++ = *folder++))
            {
            }

            mkdir((char *)buff_, 0700);
            struct stat st = {};

            u32 goodPath = 0;

            if (!stat((char *)buff_, &st))
            {
                if (folder2)
                {
                    *(buff - 1) = sep;
                    while ((*buff++ = *folder2++))
                    {
                    }

                    mkdir((char *)buff_, 0700);
                    if (!stat((char *)buff_, &st))
                    {
                        goodPath = 1;
                    }
                }
                else
                {
                    goodPath = 1;
                }
            }

            if (goodPath)
            {
                u32 n = StringLength((u08 *)buff_);
                UserSaveState_Path = PushArray(Working_Set, u08, n + 18);
                CopyNullTerminatedString((u08 *)buff_, UserSaveState_Path);
                UserSaveState_Path[n] = (u08)sep;
                UserSaveState_Name = UserSaveState_Path + n + 1;
                UserSaveState_Path[n + 17] = 0;
            }
        }
#else
        PWSTR path = 0;
        char sep = '\\';
        HRESULT hres = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &path);
        if (SUCCEEDED(hres))
        {
            PWSTR pathPtr = path;
            while ((*buff++ = *pathPtr++))
            {
            }
            *(buff - 1) = sep;
            char *folder = (char *)"PretextView";
            while ((*buff++ = *folder++))
            {
            }

            if (CreateDirectory((char *)buff_, NULL) || ERROR_ALREADY_EXISTS == GetLastError())
            {
                u32 n = StringLength((u08 *)buff_);
                UserSaveState_Path = PushArray(Working_Set, u08, n + 18);
                CopyNullTerminatedString((u08 *)buff_, UserSaveState_Path);
                UserSaveState_Path[n] = (u08)sep;
                UserSaveState_Name = UserSaveState_Path + n + 1;
                UserSaveState_Path[n + 17] = 0;
            }
        }

        if (path)
            CoTaskMemFree(path);
#endif
    }
}




// default user profile settings
global_function
void 
default_user_profile_settings()
{   
    // metadata tags color profile
    meta_outline->on = 1;

    // background color
    active_bgcolor = 0;

    // color map order
    useCustomOrder = 1;
    userColourMapOrder.nMaps = Color_Maps->nMaps;
    for (u32 i = 0; i < Color_Maps->nMaps; i++)
    {
        userColourMapOrder.order[i] = i;
    }

    // select area color
    auto_curation_state.set_default_mask_color();

    // invert mouse
    if (user_profile_settings_ptr)
    {
        user_profile_settings_ptr->invert_mouse = false;
    }

    // Grey out settings
    if (Grey_Out_Settings)
    {   
        delete Grey_Out_Settings;
        Grey_Out_Settings = new GreyOutSettings(Meta_Data);
    }
    return ;
}


global_function void
UserSaveState(const char *headerHash = "userprofile", u08 overwrite = 1, char *path = 0)
{

    if (!UserSaveState_Path)
    {
        UserSetSaveStatePaths();
    }

    char filePath[MAX_PATH_LEN];
    if (!path)
    {
        snprintf(filePath, sizeof(filePath), "%s%s", UserSaveState_Path, headerHash);
        path = filePath;
    }

    if (!overwrite)
    {
        FILE *file = fopen(path, "rb");
        if (file)
        {
            fclose(file);
        }
    }

    // Open the file for writing
    FILE *file = fopen(path, "wb");
    if (!file)
    {
        return;
    }

    u16 ngphextensions = 0;
    TraverseLinkedList(Extensions.head, extension_node)
    {
        switch (node->type)
        {
            case extension_graph:
            {
                graph *gph = (graph *)node->extension;
                ngphextensions++;
            }
        }
    }
    fwrite(&ngphextensions, sizeof(ngphextensions), 1, file);

    // Specific color for each track
    TraverseLinkedList(Extensions.head, extension_node)
    {
        switch (node->type)
        {
        case extension_graph:
        {
            graph *gph = (graph *)node->extension;

            // Get the name length and write it first
            u32 name_len = strlen((char *)gph->name);
            fwrite(&name_len, sizeof(name_len), 1, file);

            // Write the name string data
            fwrite(gph->name, sizeof(char), name_len, file);

            // Write the color data
            u08 colour = (u08)gph->activecolor;
            fwrite(&colour, sizeof(colour), 1, file);
        }
        }
    }

    // metadata tags color profile
    fwrite(&meta_data_curcolorProfile, sizeof(meta_data_curcolorProfile), 1, file);

    // metadata tags outline
    fwrite(&meta_outline->on, sizeof(meta_outline->on), 1, file);

    // backgound color
    fwrite(&active_bgcolor, sizeof(active_bgcolor), 1, file);

    // Save custom color map order
    fwrite(&useCustomOrder, sizeof(useCustomOrder), 1, file);
    fwrite(&userColourMapOrder.nMaps, sizeof(userColourMapOrder.nMaps), 1, file);
    fwrite(userColourMapOrder.order, sizeof(u32), userColourMapOrder.nMaps, file);

    // save selected area color 
    fwrite(&auto_curation_state.mask_color, sizeof(auto_curation_state.mask_color), 1, file);

    // save grey out flags
    if (Grey_Out_Settings)
    {
        fwrite(Grey_Out_Settings->grey_out_flags, sizeof(int32_t), 64, file);
    }
    else
    {
        int32_t grey_out_flags[64] = {0};
        fwrite(grey_out_flags, sizeof(int32_t), 64, file);
    }

    // save user profile settings
    if (user_profile_settings_ptr)
    {
        fwrite(&user_profile_settings_ptr->invert_mouse, sizeof(bool), 1, file);
    }
    else 
    {   
        bool invert_mouse_tmp = false;
        fwrite(&invert_mouse_tmp, sizeof(bool), 1, file);
    }

    // END of SAVE user profile settings

    fclose(file);

    printf("[UserProfile]: Saved to: %s\n", path);
}

global_function
    u08
    UserLoadState(const char *headerHash = "userprofile", const char *path = 0)
{

    if (!UserSaveState_Path)
    {
        UserSetSaveStatePaths();
    }

    char filePath[MAX_PATH_LEN];
    if (!path)
    {
        // Concatenate the save path and headerHash (filename)
        snprintf(filePath, sizeof(filePath), "%s%s", UserSaveState_Path, headerHash);
        path = filePath;
    }

    // Open the file for reading
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        return 1; // Failed to open file
    }

    // Read user profile settings
    TraverseLinkedList(Extensions.head, extension_node)
    {
        switch (node->type)
        {
        case extension_graph:
        {
            graph *gph = (graph *)node->extension;

            gph->colour = graphColors[activeGraphColour];
        }
        break;
        }
    }
    // u08 waypointsVisible;
    // u08 contigLabels;

    u16 ngphextensions;
    fread(&ngphextensions, sizeof(ngphextensions), 1, file);
    u08 colour;
    u32 *name;

    ForLoop(ngphextensions)
    {
        u32 name_len;
        if (fread(&name_len, sizeof(name_len), 1, file) != 1)
        {
            break;
        }

        char *name = (char *)malloc(name_len + 1);
        if (fread(name, sizeof(char), name_len, file) != name_len)
        {
            free(name);
            break;
        }
        name[name_len] = '\0';

        u08 colour;
        if (fread(&colour, sizeof(colour), 1, file) != 1)
        {
            free(name);
            break;
        }

        // Search for the graph in the current list and update its color
        TraverseLinkedList(Extensions.head, extension_node)
        {
            switch (node->type)
            {
            case extension_graph:
            {
                graph *gph = (graph *)node->extension;

                if (strcmp((char *)gph->name, name) == 0)
                {
                    gph->activecolor = colour;
                    gph->colour = graphColors[gph->activecolor];
                    break;
                }
            }
            }
        }

        free(name);
    }

    // metadata tags color profile
    fread(&meta_data_curcolorProfile, sizeof(meta_data_curcolorProfile), 1, file);

    // metadata tags outline
    fread(&meta_outline->on, sizeof(meta_outline->on), 1, file);

    // background color
    fread(&active_bgcolor, sizeof(active_bgcolor), 1, file);

    // Load custom color map order
    fread(&useCustomOrder, sizeof(useCustomOrder), 1, file);
    fread(&userColourMapOrder.nMaps, sizeof(userColourMapOrder.nMaps), 1, file);
    fread(userColourMapOrder.order, sizeof(u32), userColourMapOrder.nMaps, file);

    // load selected area color 
    f32 tmp_mask_color[4];
    if (fread(tmp_mask_color, sizeof(tmp_mask_color), 1, file) == 1)
    {
        auto_curation_state.set_mask_color(tmp_mask_color);
    }
    else 
    {
        auto_curation_state.set_mask_color(auto_curation_state.mask_color_default);
    }

    // load grey out flags
    int32_t grey_out_flags[64];
    if (fread(grey_out_flags, sizeof(int32_t), 64, file) == 64)
    {   
        if (Grey_Out_Settings)
        {
            memcpy(Grey_Out_Settings->grey_out_flags, grey_out_flags, 64 * sizeof(int32_t));
        }
    }

    // load user profile settings
    bool invert_mouse_tmp;
    if (fread(&invert_mouse_tmp, sizeof(bool), 1, file) == 1)
    {
        if (user_profile_settings_ptr)
        {
            user_profile_settings_ptr->invert_mouse = invert_mouse_tmp;
        }
    }



    fclose(file);
    return 0; // Success
}



global_variable
u32
Global_Text_Buffer[1024] = {0};

global_variable
u32
Global_Text_Buffer_Ptr = 0;

global_function
void
TextInput(GLFWwindow* window, u32 codepoint)
{
    if (!GatheringTextInput)
    {
        (void)window;
        
        Global_Text_Buffer[Global_Text_Buffer_Ptr++] = codepoint;
        
        if (Global_Text_Buffer_Ptr == ArrayCount(Global_Text_Buffer)) Global_Text_Buffer_Ptr = 0;
        //nk_input_unicode(NK_Context, codepoint); 
    }
}

global_function
void
CopyEditsToClipBoard(GLFWwindow *window)
{
    u32 nEdits = my_Min(Edits_Stack_Size, Map_Editor->nEdits);

    if (nEdits)
    {
        u32 bufferSize = 256 * nEdits;
        u08 *buffer = PushArray(Working_Set, u08, bufferSize);
        u32 bufferPtr = 0;

        u32 editStackPtr = Map_Editor->editStackPtr == nEdits ? 0 : Map_Editor->editStackPtr;

        f64 bpPerPixel = (f64)Total_Genome_Length / (f64)(Number_of_Textures_1D * Texture_Resolution);

        ForLoop(nEdits)
        {
            if (editStackPtr == nEdits)
            {
                editStackPtr = 0;
            }

            map_edit *edit = Map_Editor->edits + editStackPtr++;

            u32 start = my_Min(edit->finalPix1, edit->finalPix2);
            u32 end = my_Max(edit->finalPix1, edit->finalPix2);
            u32 to = start ? start - 1 : (end < (Number_of_Pixels_1D - 1) ? end + 1 : end);

            u32 oldFrom = Map_State->contigRelCoords[start];
            u32 oldTo = Map_State->contigRelCoords[end];
            u32 *name1 = (Original_Contigs + Map_State->get_original_contig_id(start))->name;
            u32 *name2 = (Original_Contigs + Map_State->get_original_contig_id(end))->name;
            u32 *newName = (Original_Contigs + Map_State->get_original_contig_id(to))->name;

            bufferPtr += (u32)stbsp_snprintf((char *)buffer + bufferPtr, (s32)(bufferSize - bufferPtr), "Edit %d:\n       %s[%$.2fbp] to %s[%$.2fbp]\n%s\n       %s[%$.2fbp]\n",
                    index + 1, (char *)name1, (f64)oldFrom * bpPerPixel, (char *)name2, (f64)oldTo * bpPerPixel,
                    edit->finalPix1 > edit->finalPix2 ? (const char *)"       inverted and moved to" : (const char *)"       moved to",
                    (char *)newName, (f64)Map_State->contigRelCoords[to] * bpPerPixel);
        }

        buffer[bufferPtr] = 0;
        glfwSetClipboardString(window, (const char *)buffer);
        
        FreeLastPush(Working_Set); // buffer
    }
}

global_function
void
GenerateAGP(char *path, u08 overwrite, u08 formatSingletons, u08 preserveOrder)
{   

    // 这里粘贴所有的original contigs
    u32* tmp_orignal_contig_ids = new u32[Number_of_Pixels_1D];
    for (u32 i = 0 ; i< Number_of_Pixels_1D; i++ )tmp_orignal_contig_ids[i] = Map_State->originalContigIds[i];
    Map_State->restore_cutted_contigs_all(Number_of_Pixels_1D);
    UpdateContigsFromMapState(); // todo 检查粘贴后会不会影响 painted 的 scaffolds

    FILE *file;
    if (!overwrite && (file = fopen((const char *)path, "rb")))
    {
        fclose(file);
        return;
    }

    if ((file = fopen((const char *)path, "w")))
    {
        char buffer[1024];

        stbsp_snprintf(buffer, sizeof(buffer), "##agp-version\t%s\n# DESCRIPTION: Generated by %s\n# HiC MAP RESOLUTION: %f bp/texel\n", formatSingletons ? "<NA>" : "2.1", PretextView_Version, (f64)Total_Genome_Length / (f64)Number_of_Pixels_1D);
        fwrite(buffer, 1, strlen(buffer), file);

        if (formatSingletons)
        {
            stbsp_snprintf(buffer, sizeof(buffer), "# WARNING: Non-standard AGP, singletons may be reverse-complemented\n");
            fwrite(buffer, 1, strlen(buffer), file);
        }

        u32 scaffId = 0;
        u64 scaffCoord_Start = 1;
        u32 scaffPart = 0;

        u32 scaffId_unPainted = 0;
        if (preserveOrder) ForLoop(Contigs->numberOfContigs) scaffId_unPainted = my_Max(scaffId_unPainted, (Contigs->contigs_arr + index)->scaffId);

        for (   u08 type = 0;
                type < (preserveOrder ? 1 : 2);
                ++type)
        {
            ForLoop(Contigs->numberOfContigs)
            {
                contig *cont = Contigs->contigs_arr + index;
                u08 invert = IsContigInverted(index);
                u32 startCoord = cont->startCoord - (invert ? (cont->length - 1) : 0);

                invert = (!formatSingletons && (!cont->scaffId || (index && (index < (Contigs->numberOfContigs - 1)) && (cont->scaffId != ((cont + 1)->scaffId)) && (cont->scaffId != ((cont - 1)->scaffId))) || (!index && (cont->scaffId != ((cont + 1)->scaffId))) || ((index == (Contigs->numberOfContigs - 1)) && (cont->scaffId != ((cont - 1)->scaffId))))) ? 0 : invert;
                
                u64 contRealStartCoord = (u64)((f64)startCoord / (f64)Number_of_Pixels_1D * (f64)Total_Genome_Length) + 1;
                u64 contRealEndCoord = (u64)((f64)(startCoord + cont->length) / (f64)Number_of_Pixels_1D * (f64)Total_Genome_Length);
                u64 contRealSize = contRealEndCoord - contRealStartCoord + 1;

                char *contName = (char *)((Original_Contigs + cont->get_original_contig_id())->name);

                if (cont->scaffId && !type)
                {
                    if (cont->scaffId != scaffId)
                    {
                        scaffId = cont->scaffId;
                        scaffCoord_Start = 1;
                        scaffPart = 0;
                    }

                    u64 scaffCoord_End;

                    if (scaffPart)
                    {
                        scaffCoord_End = scaffCoord_Start + 99;
                        stbsp_snprintf(buffer, sizeof(buffer), "Scaffold_%u\t%" PRIu64 "\t%" PRIu64 "\t%u\tU\t100\tscaffold\tyes\tproximity_ligation\n", scaffId, scaffCoord_Start, scaffCoord_End, ++scaffPart);
                        fwrite(buffer, 1, strlen(buffer), file);

                        scaffCoord_Start = scaffCoord_End + 1;
                    }

                    scaffCoord_End = scaffCoord_Start + contRealSize - 1;
                    stbsp_snprintf(buffer, sizeof(buffer), "Scaffold_%u\t%" PRIu64 "\t%" PRIu64 "\t%u\tW\t%s\t%" PRIu64 "\t%" PRIu64 "\t%s\tPainted", scaffId, scaffCoord_Start, scaffCoord_End, ++scaffPart, contName, contRealStartCoord, contRealEndCoord, invert ? "-" : "+");
                    fwrite(buffer, 1, strlen(buffer), file);
                    if (*cont->metaDataFlags)
                    {
                        ForLoop2(ArrayCount(Meta_Data->tags))
                        {
                            if (*cont->metaDataFlags & (1 << index2))
                            {
                                stbsp_snprintf(buffer, sizeof(buffer), "\t%s", (char *)Meta_Data->tags[index2]);
                                fwrite(buffer, 1, strlen(buffer), file);
                            }
                        }
                    }
                    fwrite("\n", 1, 1, file);
                    scaffCoord_Start = scaffCoord_End + 1;
                }
                else if (!cont->scaffId && (preserveOrder || type))
                {
                    stbsp_snprintf(buffer, sizeof(buffer), "Scaffold_%u\t1\t%" PRIu64 "\t1\tW\t%s\t%" PRIu64 "\t%" PRIu64 "\t%s", (preserveOrder ? ++scaffId_unPainted : ++scaffId), contRealSize, contName, contRealStartCoord, contRealEndCoord, invert ? "-" : "+");
                    fwrite(buffer, 1, strlen(buffer), file);
                    if (*cont->metaDataFlags)
                    {
                        ForLoop2(ArrayCount(Meta_Data->tags))
                        {
                            if (*cont->metaDataFlags & (1 << index2))
                            {
                                stbsp_snprintf(buffer, sizeof(buffer), "\t%s", (char *)Meta_Data->tags[index2]);
                                fwrite(buffer, 1, strlen(buffer), file);
                            }
                        }
                    }
                    fwrite("\n", 1, 1, file);
                }
            }
        }

        fclose(file);
    }

    // 把临时切开的再粘贴上
    for (u32 i = 0; i < Number_of_Pixels_1D; i++) Map_State->originalContigIds[i] = tmp_orignal_contig_ids[i]; 
    UpdateContigsFromMapState();
    delete[] tmp_orignal_contig_ids;
    return;
}


// global_function
// void
// SortMapByMetaTags()
// {
//     u32 nPixelToConsider = Number_of_Pixels_1D;
//     for (;;)
//     {
//         u64 maxFlag = 0;
//         ForLoop(nPixelToConsider) maxFlag = my_Max(maxFlag, Map_State->metaDataFlags[index]);
//         if (!maxFlag) break;

//         ForLoop(nPixelToConsider)
//         {
//             u32 pixelEnd = nPixelToConsider - index - 1;
//             s32 delta = (s32)(Number_of_Pixels_1D - pixelEnd - 1);
//             if (Map_State->metaDataFlags[pixelEnd] == maxFlag)
//             {
//                 u32 pixelStart = pixelEnd;
//                 while (pixelStart && Map_State->metaDataFlags[pixelStart - 1] == maxFlag) --pixelStart;
                
//                 if (delta)
//                 {
//                     RearrangeMap(pixelStart, pixelEnd, delta);
//                     AddMapEdit(delta, {(u32)((s32)pixelStart + delta), (u32)((s32)pixelEnd + delta)}, 0);
//                 }

//                 nPixelToConsider -= (pixelEnd - pixelStart + 1);
//                 break;
//             }
//         }
//     }
// }


void SortMapByMetaTags(u64 tagMask)
{
    u32 nPixelToConsider = Number_of_Pixels_1D;
    sortMetaEdits = 0;
    for (;;)
    {
        u64 maxFlag = 0;
        bool hasRelevantFlags = false;
        ForLoop(nPixelToConsider)
        {
            u64 relevantFlags = Map_State->metaDataFlags[index] & tagMask;
            if (relevantFlags)
            {
                maxFlag = my_Max(maxFlag, relevantFlags);
                hasRelevantFlags = true;
            }
        }
        if (!hasRelevantFlags)
            break;

        ForLoop(nPixelToConsider)
        {
            u32 pixelEnd = nPixelToConsider - index - 1;
            s32 delta = (s32)(Number_of_Pixels_1D - pixelEnd - 1);
            u64 relevantFlags = Map_State->metaDataFlags[pixelEnd] & tagMask;
            if (relevantFlags == maxFlag)
            {
                u32 pixelStart = pixelEnd;
                while (pixelStart && ((Map_State->metaDataFlags[pixelStart - 1] & tagMask) == maxFlag))
                    --pixelStart;
                if (delta)
                {
                    RearrangeMap(pixelStart, pixelEnd, delta);
                    AddMapEdit(delta, {(u32)((s32)pixelStart + delta), (u32)((s32)pixelEnd + delta)}, 0);
                    sortMetaEdits++;
                }
                nPixelToConsider -= (pixelEnd - pixelStart + 1);
                break;
            }
        }
    }
}


MainArgs 
{
    u32 initWithFile = 0;   // initialization with .map file or not 
    u08 currFile[256];      // save the name of file inputed 
    u08 *currFileName = 0;
    u64 headerHash = 0;
    if (ArgCount >= 2)
    {
        initWithFile = 1;
        CopyNullTerminatedString((u08 *)ArgBuffer[1], currFile);  // copy the filepath to currfile
        printf("Read from file: %s\n", currFile);  
    }

    Mouse_Move.x = -1.0;  // intialize the mouse position
    Mouse_Move.y = -1.0;

    Tool_Tip_Move.pixels.x = 0;      // green selection band
    Tool_Tip_Move.pixels.y = 0;      //
    Tool_Tip_Move.worldCoords.x = 0; // word with the mouse
    Tool_Tip_Move.worldCoords.y = 0; // 

    Edit_Pixels.pixels.x = 0;
    Edit_Pixels.pixels.y = 0;
    Edit_Pixels.worldCoords.x = 0;
    Edit_Pixels.worldCoords.y = 0;
    Edit_Pixels.editing = 0;
    Edit_Pixels.selecting = 0;
    Edit_Pixels.scaffSelecting = 0;
    Edit_Pixels.snap = 0;

    Camera_Position.x = 0.0f; // 0.0f
    Camera_Position.y = 0.0f; // 0.0f
    Camera_Position.z = 1.0f; // 1.0f 

    CreateMemoryArena(Working_Set, MegaByte(256));
    Thread_Pool = ThreadPoolInit(&Working_Set, 3); 

    glfwSetErrorCallback(ErrorCallback);
    if (!glfwInit()) 
    {      
        fprintf(stderr, "Failed in initializing the glfw window, exit...\n");  
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 8);

    GLFWwindow* window = glfwCreateWindow(1080, 1080, PretextView_Title, NULL, NULL);  // 1080 1080  TODO add a button to change the size of the window
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    NK_Context = PushStruct(Working_Set, nk_context);
    glfwSetWindowUserPointer(window, NK_Context);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, GLFWChangeFrameBufferSize);
    glfwSetWindowSizeCallback(window, GLFWChangeWindowSize);
    glfwSetCursorPosCallback(window, MouseMove);
    glfwSetMouseButtonCallback(window, Mouse);
    glfwSetScrollCallback(window, Scroll);
    glfwSetKeyCallback(window, KeyBoard);
    glfwSetCharCallback(window, TextInput);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MULTISAMPLE);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("OpenGL version supported by this platform (%s): \n", glGetString(GL_VERSION));

    #ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // TODO: CHECK THE PROBLEM, no idea of the exception 
    // glDebugMessageCallback( MessageCallback, 0 ); // Exception has occurred. EXC_BAD_ACCESS (code=1, address=0x0)
    #endif
    
    s32 width, height, display_width, display_height;
    glfwGetWindowSize(window, &width, &height);
    glfwGetFramebufferSize(window, &display_width, &display_height);
    Screen_Scale.x = (f32)display_width/(f32)width;
    Screen_Scale.y = (f32)display_height/(f32)height;
    
    // Cursors
    GLFWcursor *arrowCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    GLFWcursor *crossCursor = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    Setup();

    if (initWithFile)
    {
        UI_On = LoadFile((const char *)currFile, Loading_Arena, (char **)&currFileName, &headerHash) == ok ? 0 : 1;
        if (currFileName)
        {
            glfwSetWindowTitle(window, (const char *)currFileName);
            FenceIn(SetSaveStateNameBuffer((char *)currFileName));
        }
    }
    else
    {
        UI_On = 1;
    }

    // if (1)
    // {   
    //     bool show_flag = true;
    //     auto tmp = new TexturesArray4AI(32, 1024, (char*)"123", Contigs);
    //     tmp->copy_buffer_to_textures(Contact_Matrix, show_flag);
    //     delete tmp;
    //     if (show_flag)
    //     {
    //         glfwSetWindowShouldClose(window, false);
    //     }
    // }

    // file browser
    struct file_browser browser;
    struct file_browser saveBrowser;
    struct file_browser loadBrowser;
    struct file_browser saveAGPBrowser;

    u32 showClearCacheScreen = 0;

    struct media media;
    {
        media.icons.home = IconLoad(IconHome, IconHome_Size);
        media.icons.directory = IconLoad(IconFolder, IconFolder_Size);
        media.icons.computer = IconLoad(IconDrive, IconDrive_Size);
        media.icons.default_file = IconLoad(IconFile, IconFile_Size);
        media.icons.img_file = IconLoad(IconImage, IconImage_Size);
        MediaInit(&media);
        FileBrowserInit(&browser, &media);
        FileBrowserInit(&saveBrowser, &media);
        FileBrowserInit(&loadBrowser, &media);
        FileBrowserInit(&saveAGPBrowser, &media);
    }
    
    {
        f64 mousex, mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        MouseMove(window, mousex, mousey);
    }
    
    if (UserLoadState("userprofile", 0)!=0) 
        InitializeColorMapOrder();

    Redisplay = 1;

    char searchbuf[256] = {0};
    s32 caseSensitive_search_sequences = 0;

    while (!glfwWindowShouldClose(window)) 
    {
        if (Redisplay) 
        {
            Render();
            glfwSwapBuffers(window);
            Redisplay = 0;
            if (currFileName) SaveState(headerHash);
        }

        if (Loading) 
        {
            if (currFileName) SaveState(headerHash);
            LoadFile((const char *)currFile, Loading_Arena, (char **)&currFileName, &headerHash);
            if (currFileName)
            {
                glfwSetWindowTitle(window, (const char *)currFileName);
                FenceIn(SetSaveStateNameBuffer((char *)currFileName));
            }
            glfwPollEvents(); 
            Loading = 0;
            Redisplay = 1;
        }

        if (auto_sort_state)
        {
            if (currFileName) SaveState(headerHash);
            auto_sort_func((char*)currFileName);
            auto_sort_state = 0;
            Redisplay = 1;
        }

        if (auto_cut_state)
        {   
            if (currFileName) SaveState(headerHash);
            auto_cut_func((char*)currFileName, Loading_Arena);
            auto_cut_state = 0;
            Redisplay = 1;
        }

        if (UI_On) 
        {
            glfwSetCursor(window, arrowCursor);
            f64 x, y;
            nk_input_begin(NK_Context);
            GatheringTextInput = 1;
            glfwPollEvents();

            /*nk_input_key(NK_Context, NK_KEY_DEL, glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_ENTER, glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_TAB, glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_BACKSPACE, glfwGetKey(window, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_LEFT, glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_RIGHT, glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_UP, glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS);
            nk_input_key(NK_Context, NK_KEY_DOWN, glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS);

            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
            {
                nk_input_key(NK_Context, NK_KEY_COPY, glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_PASTE, glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_CUT, glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_CUT, glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
                nk_input_key(NK_Context, NK_KEY_SHIFT, 1);
            } 
            else
            {
                nk_input_key(NK_Context, NK_KEY_COPY, 0);
                nk_input_key(NK_Context, NK_KEY_PASTE, 0);
                nk_input_key(NK_Context, NK_KEY_CUT, 0);
                nk_input_key(NK_Context, NK_KEY_SHIFT, 0);
            }*/
            
            nk_input_scroll(NK_Context, NK_Scroll);
            NK_Scroll.x = 0;
            NK_Scroll.y = 0;

            glfwGetCursorPos(window, &x, &y);
            x *= (f64)Screen_Scale.x;
            y *= (f64)Screen_Scale.y;

            nk_input_motion(NK_Context, (s32)x, (s32)y);
            nk_input_button(NK_Context, NK_BUTTON_LEFT, (s32)x, (s32)y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
            nk_input_button(NK_Context, NK_BUTTON_MIDDLE, (s32)x, (s32)y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
            nk_input_button(NK_Context, NK_BUTTON_RIGHT, (s32)x, (s32)y, glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
          
            ForLoop(NK_Pressed_Keys_Ptr) nk_input_key(NK_Context, NK_Pressed_Keys[index], 1);
            NK_Pressed_Keys_Ptr = 0;
            
            ForLoop(Global_Text_Buffer_Ptr) nk_input_unicode(NK_Context, Global_Text_Buffer[index]);
            Global_Text_Buffer_Ptr = 0;

            GatheringTextInput = 0;
            nk_input_end(NK_Context);

            s32 showFileBrowser = 0;
            s32 showAboutScreen = 0;
            s32 showSaveStateScreen = 0;
            s32 showLoadStateScreen = 0;
            s32 showSaveAGPScreen = 0;
            s32 showMetaDataTagEditor = 0;
            s32 showUserProfileScreen = 0;
            static u32 currGroup1 = 0;
            static u32 currGroup2 = 0;
            static s32 currSelected1 = -1;
            static s32 currSelected2 = -1;
            {
                static nk_flags flags = NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_SCROLL_AUTO_HIDE | NK_WINDOW_TITLE;

                if (nk_begin_titled(NK_Context, "Options", currFileName ? (const char *)currFileName : "", nk_rect(Screen_Scale.x * 10, Screen_Scale.y * 10, Screen_Scale.x * 800, Screen_Scale.y * 600), flags))
                {
                    struct nk_rect bounds = nk_window_get_bounds(NK_Context);
                    bounds.w /= 8;
                    bounds.h /= 8;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 400), bounds))
                    {
                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                        theme curr = Current_Theme;
                        ForLoop(THEME_COUNT)
                        {
                            curr = nk_option_label(NK_Context, (const char *)Theme_Name[index], curr == (theme)index) ? (theme)index : curr;
                        }
                        if (curr != Current_Theme)
                        {
                            SetTheme(NK_Context, curr);
                        }

                        nk_contextual_end(NK_Context); 
                    } 

                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 5);
                    showFileBrowser = nk_button_label(NK_Context, "Load Map");
                    if (currFileName)
                    {
                        showSaveStateScreen = nk_button_label(NK_Context, "Save State");
                        showLoadStateScreen = nk_button_label(NK_Context, "Load State");
                        showSaveAGPScreen = nk_button_label(NK_Context, "Generate AGP");
                        if (nk_button_label(NK_Context, "Clear Cache")) showClearCacheScreen = 1; // used to clear cache for current opened sample
                    }
                    showUserProfileScreen = nk_button_label(NK_Context, "User Profile");
                    showAboutScreen = nk_button_label(NK_Context, "About");

                    // AI & Pixel Sort button 
                    struct nk_color sort_button_normal = nk_rgb(153, 50, 204); 
                    struct nk_color sort_button_hover  = nk_rgb(186, 85, 211); 
                    struct nk_color sort_button_active = nk_rgb(128, 0, 128);  
                    push_nk_style(NK_Context, sort_button_normal, sort_button_hover, sort_button_active);
                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3);

                    // AI sort button
                    /*
                    {
                        if (ai_sort_button && currFileName)
                        {   
                            printf("AISort button clicked\n");
                            // check if the ai model is loaded
                            if (!frag_sort_method) 
                            {
                                printf("Loading AI Model...\n");
                                frag_sort_method = std::make_unique<FragSortTool>();
                            }

                            // prepare the graph data 
                            auto texture_array_4_ai = TexturesArray4AI(Number_of_Textures_1D, Texture_Resolution, (char*)currFileName, Contigs);
                            texture_array_4_ai.copy_buffer_to_textures(Contact_Matrix);
                            texture_array_4_ai.cal_compressed_hic(Contigs, Extensions);
                            FragsOrder frags_order(texture_array_4_ai.get_num_frags()); // intilize the frags_order with the number of fragments including the filtered out ones
                            bool ai_flag = false;
                            if (ai_flag)
                            {
                                GraphData* graph_data;
                                texture_array_4_ai.cal_graphData(graph_data);
                                frag_sort_method->cal_curated_fragments_order( graph_data, frags_order);
                                delete graph_data;
                            }
                            else 
                            {
                                std::vector<std::string> exclude_tags = {"haplotig", "unloc"};
                                std::vector<s32> exclude_frag_idx = get_exclude_metaData_idx(exclude_tags);
                                LikelihoodTable likelihood_table(
                                    texture_array_4_ai.get_frags(), 
                                    texture_array_4_ai.get_compressed_hic(), 
                                    (f32)auto_curation_state.smallest_frag_size_in_pixel / ((f32)Number_of_Pixels_1D + 1.f), 
                                    exclude_frag_idx);
                                // use the compressed_hic to calculate the frags_order directly
                                frag_sort_method->sort_according_likelihood_dfs( likelihood_table, frags_order, 0.4, texture_array_4_ai.get_frags());
                            }
                            std::cout << std::endl;
                        }
                    }
                    */
                    // Pixel Cut
                    {
                        bounds = nk_widget_bounds(NK_Context);
                        auto_cut_button = nk_button_label(NK_Context, "Pixel Cut");

                        if (auto_cut_button && currFileName)
                        {
                            auto_cut_state = 1;
                            auto_curation_state.clear(); // if click the button, the sort/cut will be applied globally
                        }
                        // window to set the parameters for cut
                        if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 480, Screen_Scale.y * 800), bounds))
                        {   
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);

                            nk_label(NK_Context, "Cut threshold (default: 0.05)", NK_TEXT_LEFT);
                            nk_edit_string_zero_terminated(
                                NK_Context, 
                                NK_EDIT_FIELD, 
                                (char*)auto_curation_state.auto_cut_threshold_buf, 
                                sizeof(auto_curation_state.auto_cut_threshold_buf), 
                                nk_filter_float);
                            
                            nk_label(NK_Context, "Pixel_mean window size (default: 8)", NK_TEXT_LEFT); //  见鬼了，见鬼了，这个地方，如果我使用 "Pixel_mean windows (default: 8)" 点击弹出的窗口外面程序就会卡死，但是使用"Pixel_mean window size (default: 8)" 就不会卡死
                            nk_edit_string_zero_terminated(
                                NK_Context, 
                                NK_EDIT_FIELD, 
                                (char*)auto_curation_state.auto_cut_diag_window_for_pixel_mean_buf, 
                                sizeof(auto_curation_state.auto_cut_diag_window_for_pixel_mean_buf), 
                                nk_filter_decimal);

                            nk_label(NK_Context, "Smallest frag size (default: 8)", NK_TEXT_LEFT);
                            nk_edit_string_zero_terminated(
                                NK_Context, 
                                NK_EDIT_FIELD, 
                                (char*)auto_curation_state.auto_cut_smallest_frag_size_in_pixel_buf, 
                                sizeof(auto_curation_state.auto_cut_smallest_frag_size_in_pixel_buf), 
                                nk_filter_decimal);

                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                            if (nk_button_label(NK_Context, "Cancel")) 
                            {   
                                printf("[Pixel Cut] Cancel button clicked\n");
                                auto_curation_state.set_buf();
                                nk_contextual_close(NK_Context);
                            }
                            if (nk_button_label(NK_Context, "Apply")) 
                            {
                                // Apply changes
                                // Convert text to integer and float
                                auto_curation_state.update_value_from_buf(); // todo 这里还有问题，因为设置了pixel_mean之后 并没有重新计算 pixel_density...
                                
                                nk_contextual_close(NK_Context);

                                auto_curation_state.set_buf();
                                fmt::print("[Pixel Cut] cut_threshold:               {:.3f}\n", auto_curation_state.auto_cut_threshold);
                                fmt::print("[Pixel Cut] pixel mean window size:      {}\n", auto_curation_state.auto_cut_diag_window_for_pixel_mean);
                                fmt::print("[Pixel Cut] smallest_frag_size_in_pixel: {}\n", auto_curation_state.auto_cut_smallest_frag_size_in_pixel);
                            }

                            nk_contextual_end(NK_Context);

                            
                        }
                    }

                    // Pixel Sort button
                    bounds = nk_widget_bounds(NK_Context);
                    auto_sort_button = nk_button_label(NK_Context, "Pixel Sort");
                    {   
                        if (auto_sort_button && currFileName) 
                        {
                            auto_sort_state = 1;
                            auto_curation_state.clear(); // if click the button, the sort/cut will be applied globally
                        }
                        // window to set the parameters for sort
                        if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 480, Screen_Scale.y * 400), bounds))
                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                            nk_label(NK_Context, "Smallest Frag Size (default: 2)", NK_TEXT_LEFT);
                            nk_edit_string_zero_terminated(
                                NK_Context, 
                                NK_EDIT_FIELD, 
                                (char*)auto_curation_state.frag_size_buf, 
                                sizeof(auto_curation_state.frag_size_buf), 
                                nk_filter_decimal);

                            nk_label(NK_Context, "Link threshold (default: 0.4)", NK_TEXT_LEFT);
                            nk_edit_string_zero_terminated(
                                NK_Context, 
                                NK_EDIT_FIELD, 
                                (char*)auto_curation_state.score_threshold_buf, 
                                sizeof(auto_curation_state.score_threshold_buf), 
                                nk_filter_float);
                            
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3);
                            if (nk_option_label(NK_Context, "UnionFind", auto_curation_state.sort_mode == 0))
                            {
                                auto_curation_state.sort_mode = 0;
                            }

                            if (nk_option_label(NK_Context, "Fuse", auto_curation_state.sort_mode == 1)) 
                            {
                                auto_curation_state.sort_mode = 1;
                            }

                            if (nk_option_label(NK_Context, "Deep Fuse", auto_curation_state.sort_mode == 2)) 
                            {
                                auto_curation_state.sort_mode = 2;
                            }

                            // if (nk_option_label(NK_Context, "YaHS", auto_curation_state.sort_mode == 3)) 
                            // {
                            //     auto_curation_state.sort_mode = 3;
                            // }

                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                            if (nk_button_label(NK_Context, "Cancel")) 
                            {   
                                printf("[Pixel Sort] Cancel button clicked\n");
                                auto_curation_state.set_buf();
                                nk_contextual_close(NK_Context);
                            }
                            if (nk_button_label(NK_Context, "Apply")) 
                            {
                                // Apply changes
                                // Convert text to integer and float
                                auto_curation_state.update_value_from_buf();

                                nk_contextual_close(NK_Context);

                                auto_curation_state.set_buf();
                                fmt::print("[Pixel Sort] smallest_frag_size_in_pixel: {}\n", auto_curation_state.smallest_frag_size_in_pixel);
                                fmt::print("[Pixel Sort] link_score_threshold:        {:.3f}\n", auto_curation_state.link_score_threshold);
                                fmt::print("[Pixel Sort] Sort mode:                   {}\n", auto_curation_state.get_sort_mode_name());
                            }
                            // redo all the changes
                            if (!auto_curation_state.show_autoSort_redo_confirm_popup)
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                if (nk_button_label(NK_Context, "Redo All (Careful!)")) 
                                {   
                                    auto_curation_state.show_autoSort_redo_confirm_popup=true;
                                }
                            } else
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                nk_label(NK_Context, "Are you sure to redo all edits?", NK_TEXT_LEFT);
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                                if (nk_button_label(NK_Context, "Yes")) 
                                {
                                    auto_curation_state.show_autoSort_redo_confirm_popup=false;
                                    printf("[Pixel Sort] Redo all edits.\n");
                                    RedoAllEdits(Map_Editor);
                                }
                                if (nk_button_label(NK_Context, "No")) 
                                {
                                    auto_curation_state.show_autoSort_redo_confirm_popup=false;
                                }
                            }
                            // reject all the edits
                            if (!auto_curation_state.show_autoSort_erase_confirm_popup)
                            {   
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                if (nk_button_label(NK_Context, "Erase All (Careful!)")) 
                                {   
                                    auto_curation_state.show_autoSort_erase_confirm_popup=true;
                                }
                            } else
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                nk_label(NK_Context, "Are you sure to erase all edits?", NK_TEXT_LEFT);
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                                if (nk_button_label(NK_Context, "Yes")) 
                                {
                                    auto_curation_state.show_autoSort_erase_confirm_popup=false;
                                    printf("[Pixel Sort] Erase all edits.\n");
                                    EraseAllEdits(Map_Editor);
                                }
                                if (nk_button_label(NK_Context, "No")) 
                                {
                                    auto_curation_state.show_autoSort_erase_confirm_popup=false;
                                }
                            }
                            nk_contextual_end(NK_Context);
                        }
                    }
                    pop_nk_style(NK_Context, 3); // pop the style for Pixel Cut and Pixel Sort button

                    // waypoint edit mode
                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3);
                    
                    bounds = nk_widget_bounds(NK_Context);

                    if (nk_option_label(NK_Context, "Waypoint Edit Mode", Waypoint_Edit_Mode) != Waypoint_Edit_Mode) {
                        Global_Mode = Waypoint_Edit_Mode ? mode_normal : mode_waypoint_edit; // 
                    }
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 750, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = Waypoint_Mode_Data->text;
                        struct nk_colorf colour_bg = Waypoint_Mode_Data->bg;
                        struct nk_colorf colour_base = Waypoint_Mode_Data->base;
                        struct nk_colorf colour_select = Waypoint_Mode_Data->selected;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Waypoint Mode Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 4);
                        nk_label(NK_Context, "Base", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Selected", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 4);
                        colour_base = nk_color_picker(NK_Context, colour_base, NK_RGBA);
                        colour_select = nk_color_picker(NK_Context, colour_select, NK_RGBA);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 4);
                        if (nk_button_label(NK_Context, "Default")) colour_base = Red_Full;
                        if (nk_button_label(NK_Context, "Default")) colour_select = Blue_Full;
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Waypoint_Mode_Data->text = colour_text;
                        Waypoint_Mode_Data->bg = colour_bg;
                        Waypoint_Mode_Data->selected = colour_select;
                        Waypoint_Mode_Data->base = colour_base;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 1.0f, &Waypoint_Mode_Data->size, 2.0f * DefaultWaypointSize, 4.0f);
                        if (nk_button_label(NK_Context, "Default")) Waypoint_Mode_Data->size = DefaultWaypointSize;

                        nk_contextual_end(NK_Context);
                    }

                    bounds = nk_widget_bounds(NK_Context);
                    if ((nk_option_label(NK_Context, "Scaffold Edit Mode", Global_Mode == mode_scaff_edit) ? 1 : 0) != (Global_Mode == mode_scaff_edit ? 1 : 0)) Global_Mode = Global_Mode == mode_scaff_edit ? mode_normal : mode_scaff_edit;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = Scaff_Mode_Data->text;
                        struct nk_colorf colour_bg = Scaff_Mode_Data->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Scaffold Mode Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 2);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Scaff_Mode_Data->text = colour_text;
                        Scaff_Mode_Data->bg = colour_bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 1.0f, &Scaff_Mode_Data->size, 2.0f * DefaultScaffSize, 4.0f);
                        if (nk_button_label(NK_Context, "Default")) Scaff_Mode_Data->size = DefaultScaffSize;

                        nk_contextual_end(NK_Context);
                    }

                    bounds = nk_widget_bounds(NK_Context);
                    if ((nk_option_label(NK_Context, "Edit Mode", Global_Mode == mode_edit) ? 1 : 0) != (Global_Mode == mode_edit ? 1 : 0)) Global_Mode = Global_Mode == mode_edit ? mode_normal : mode_edit;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 750, Screen_Scale.y * 400), bounds))
                    {
                        struct nk_colorf colour_text = Edit_Mode_Colours->fg;
                        struct nk_colorf colour_bg = Edit_Mode_Colours->bg;
                        struct nk_colorf colour_preSelect = Edit_Mode_Colours->preSelect;
                        struct nk_colorf colour_select = Edit_Mode_Colours->select;
                        struct nk_colorf colour_invSelect = Edit_Mode_Colours->invSelect;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Edit Mode Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 5);
                        nk_label(NK_Context, "Select", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Edit", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Invert", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 5);
                        colour_preSelect = nk_color_picker(NK_Context, colour_preSelect, NK_RGBA);
                        colour_select = nk_color_picker(NK_Context, colour_select, NK_RGBA);
                        colour_invSelect = nk_color_picker(NK_Context, colour_invSelect, NK_RGBA);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 5);
                        if (nk_button_label(NK_Context, "Default")) colour_preSelect = Green_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_select = Blue_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_invSelect = Red_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Edit_Mode_Colours->fg = colour_text;
                        Edit_Mode_Colours->bg = colour_bg;
                        Edit_Mode_Colours->preSelect = colour_preSelect;
                        Edit_Mode_Colours->select = colour_select;
                        Edit_Mode_Colours->invSelect = colour_invSelect;

                        nk_contextual_end(NK_Context);
                    }
                    if ((nk_option_label(NK_Context, "Select sort area", Global_Mode == mode_selectExclude_sort_area) ? 1 : 0) != (Global_Mode == mode_selectExclude_sort_area ? 1 : 0))
                        Global_Mode = (Global_Mode == mode_selectExclude_sort_area ? mode_normal : mode_selectExclude_sort_area);

                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                    Waypoints_Always_Visible = nk_check_label(NK_Context, "Waypoints Always Visible", (s32)Waypoints_Always_Visible) ? 1 : 0;
                    Scaffs_Always_Visible = nk_check_label(NK_Context, "Scaffolds Always Visible", (s32)Scaffs_Always_Visible) ? 1 : 0;

                    bounds = nk_widget_bounds(NK_Context);
                    Contig_Name_Labels->on = nk_check_label(NK_Context, "Contig Name Labels", (s32)Contig_Name_Labels->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = Contig_Name_Labels->fg;
                        struct nk_colorf colour_bg = Contig_Name_Labels->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Contig Name Label Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 2);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Contig_Name_Labels->fg = colour_text;
                        Contig_Name_Labels->bg = colour_bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Text Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 1.0f, &Contig_Name_Labels->size, 2.0f * DefaultNameLabelTextSize, 8.0f);
                        if (nk_button_label(NK_Context, "Default")) Contig_Name_Labels->size = DefaultNameLabelTextSize;
                        
                        nk_contextual_end(NK_Context);
                    }

                    bounds = nk_widget_bounds(NK_Context);
                    Scale_Bars->on = nk_check_label(NK_Context, "Scale Bars", (s32)Scale_Bars->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = Scale_Bars->fg;
                        struct nk_colorf colour_bg = Scale_Bars->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Scale Bar Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 2);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        if (nk_button_label(NK_Context, "Default")) colour_text = Red_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Scale_Bars->fg = colour_text;
                        Scale_Bars->bg = colour_bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Bar Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 10.0f, &Scale_Bars->size, 2.0f * DefaultScaleBarSize, 4.0f);
                        if (nk_button_label(NK_Context, "Default")) Scale_Bars->size = DefaultScaleBarSize;
                        
                        nk_contextual_end(NK_Context);
                    }

                    bounds = nk_widget_bounds(NK_Context);
                    Grid->on = nk_check_label(NK_Context, "Grid", (s32)Grid->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 150, Screen_Scale.y * 400), bounds))
                    {
                        struct nk_colorf colour_bg = Grid->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Grid Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 1);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Grid->bg = colour_bg;
                        
                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Grid Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 0, &Grid->size, 2.0f * DefaultGridSize, DefaultGridSize / 20.0f);
                        if (nk_button_label(NK_Context, "Default")) Grid->size = DefaultGridSize;
                        
                        nk_contextual_end(NK_Context);
                    }

                    bounds = nk_widget_bounds(NK_Context);
                    Contig_Ids->on = nk_check_label(NK_Context, "ID Bars", (s32)Contig_Ids->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 150, Screen_Scale.y * 400), bounds))
                    {
                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "ID Bar Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 0, &Contig_Ids->size, 2.0f * DefaultContigIdSize, DefaultContigIdSize / 20.0f);
                        if (nk_button_label(NK_Context, "Default")) Contig_Ids->size = DefaultContigIdSize;
                        
                        nk_contextual_end(NK_Context);
                    }

#ifdef Internal
                    bounds = nk_widget_bounds(NK_Context);
                    Tiles->on = nk_check_label(NK_Context, "Tiles", (s32)Tiles->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 150, Screen_Scale.y * 400), bounds))
                    {
                        struct nk_colorf colour_bg = Tiles->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Tiles Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 1);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Tiles->bg = colour_bg;

                        nk_contextual_end(NK_Context);
                    }
#endif

                    bounds = nk_widget_bounds(NK_Context);
                    Tool_Tip->on = nk_check_label(NK_Context, "Tool Tip", (s32)Tool_Tip->on) ? 1 : 0;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = Tool_Tip->fg;
                        struct nk_colorf colour_bg = Tool_Tip->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Tool Tip Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 2);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        Tool_Tip->fg = colour_text;
                        Tool_Tip->bg = colour_bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Text Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 1.0f, &Tool_Tip->size, 2.0f * DefaultToolTipTextSize, 8.0f);
                        if (nk_button_label(NK_Context, "Default")) Tool_Tip->size = DefaultToolTipTextSize;

                        nk_contextual_end(NK_Context);
                    }
                    
                    bool original_value = user_profile_settings_ptr->invert_mouse;
                    nk_checkbox_label(NK_Context, "Invert Mouse Buttons", (int*)&user_profile_settings_ptr->invert_mouse);
                    if (original_value != user_profile_settings_ptr->invert_mouse)
                    {   
                        fmt::print("[UserPofile]: Mouse_invert settings changed ({}) -> ({})\n", original_value, user_profile_settings_ptr->invert_mouse);
                        UserSaveState();
                    }
                    

                    // select current active tag
                    if (nk_tree_push(NK_Context, NK_TREE_TAB, "Select Active Tag", NK_MINIMIZED))
                    {
                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 20.0f, 4);
                        for (u32 i = 0; i < ArrayCount(Meta_Data->tags); i ++ )
                        {   
                            if (*Meta_Data->tags[i] == 0) break;
                            int tmp = ( MetaData_Active_Tag == i) ; 
                            tmp = nk_check_label(NK_Context, (char*)Meta_Data->tags[i], tmp);
                            if (tmp) MetaData_Active_Tag = i;
                        }
                        nk_tree_pop(NK_Context);
                    }

                    // grey out settings
                    if (nk_tree_push(NK_Context, NK_TREE_TAB, "Grey Out Tags", NK_MINIMIZED))
                    {
                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 20.0f, 4);
                        for (u32 i = 0; i < ArrayCount(Meta_Data->tags); i ++ )
                        {   
                            if (*Meta_Data->tags[i] == 0) break;
                            s32 tmp = Grey_Out_Settings->grey_out_flags[i];
                            nk_checkbox_label(NK_Context, (char*)Meta_Data->tags[i], Grey_Out_Settings->grey_out_flags+i);
                            if (tmp!=Grey_Out_Settings->grey_out_flags[i]) 
                            {
                                fmt::print("[UserPofile]: Grey out tags changed\n");
                                UserSaveState();
                            }
                        }
                        nk_tree_pop(NK_Context);
                    }



                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                    nk_label(NK_Context, "Gamma Min", NK_TEXT_LEFT);
                    s32 slider1 = nk_slider_float(NK_Context, 0, Color_Maps->controlPoints, 1.0f, 0.001f);
                    if (slider1)
                    {
                        Color_Maps->controlPoints[0] = my_Min(Color_Maps->controlPoints[0], Color_Maps->controlPoints[2]);
                    }

                    nk_label(NK_Context, "Gamma Mid", NK_TEXT_LEFT);
                    s32 slider2 = nk_slider_float(NK_Context, 0, Color_Maps->controlPoints + 1, 1.0f, 0.001f);

                    nk_label(NK_Context, "Gamma Max", NK_TEXT_LEFT);
                    s32 slider3 = nk_slider_float(NK_Context, 0, Color_Maps->controlPoints + 2, 1.0f, 0.001f);
                    if (slider3)
                    {
                        Color_Maps->controlPoints[2] = my_Max(Color_Maps->controlPoints[2], Color_Maps->controlPoints[0]);
                    }

                    // add a row 
                    nk_layout_row_static(NK_Context, Screen_Scale.y * 30.0f, (s32)(Screen_Scale.x * 180), 3);
                    s32 defaultGamma = nk_button_label(NK_Context, "Default Gamma");
                    if (defaultGamma)
                    {
                        Color_Maps->controlPoints[0] = 0.0f;
                        Color_Maps->controlPoints[1] = 0.5f;
                        Color_Maps->controlPoints[2] = 1.0f;
                    }
                   
                    if (slider1 || slider2 || slider3 || defaultGamma)
                    {
                        Color_Maps->controlPoints[1] = my_Min(my_Max(Color_Maps->controlPoints[1], Color_Maps->controlPoints[0]), Color_Maps->controlPoints[2]);
                        glUseProgram(Contact_Matrix->shaderProgram);
                        glUniform3fv( Color_Maps->cpLocation, 1, Color_Maps->controlPoints);
                    }

                    f32 ratio[] = {0.23f, 0.32f, NK_UNDEFINED};
                    nk_layout_row(NK_Context, NK_DYNAMIC, Screen_Scale.y * 30.0f, 3, ratio);
                    
                    showMetaDataTagEditor = nk_button_label(NK_Context, "Meta Data Tags");
                    bounds = nk_widget_bounds(NK_Context);
                    if ((nk_option_label(NK_Context, "MetaData Tag Mode", Global_Mode == mode_meta_edit) ? 1 : 0) != (Global_Mode == mode_meta_edit ? 1 : 0)) Global_Mode = Global_Mode == mode_meta_edit ? mode_normal : mode_meta_edit;
                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 300, Screen_Scale.y * 420), bounds))
                    {
                        struct nk_colorf colour_text = MetaData_Mode_Data->text;
                        struct nk_colorf colour_bg = MetaData_Mode_Data->bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "MetaData Mode Colour", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        nk_label(NK_Context, "Text", NK_TEXT_CENTERED);
                        nk_label(NK_Context, "Background", NK_TEXT_CENTERED);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 2);
                        colour_text = nk_color_picker(NK_Context, colour_text, NK_RGBA);
                        colour_bg = nk_color_picker(NK_Context, colour_bg, NK_RGBA);

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 2);
                        if (nk_button_label(NK_Context, "Default")) colour_text = Yellow_Text_Float;
                        if (nk_button_label(NK_Context, "Default")) colour_bg = Grey_Background;

                        MetaData_Mode_Data->text = colour_text;
                        MetaData_Mode_Data->bg = colour_bg;

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                        nk_label(NK_Context, "Size", NK_TEXT_CENTERED);
                        nk_slider_float(NK_Context, 1.0f, &MetaData_Mode_Data->size, 2.0f * DefaultMetaDataSize, 4.0f);
                        if (nk_button_label(NK_Context, "Default")) MetaData_Mode_Data->size = DefaultMetaDataSize;

                        nk_contextual_end(NK_Context);
                    }
                    MetaData_Always_Visible = nk_check_label(NK_Context, "MetaData Tags Always Visible", (s32)MetaData_Always_Visible) ? 1 : 0;
                    
                    if (File_Loaded)
                    {
                        // nk_layout_row_static(NK_Context, Screen_Scale.y * 30.0f, (s32)(Screen_Scale.x * 300), 1);
                        // if (nk_button_label(NK_Context, "Sort Map by Meta Data Tags")) SortMapByMetaTags();

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                        nk_label(NK_Context, "Sort Map by Meta Data Tags:", NK_TEXT_ALIGN_LEFT);
                        static int NUM_TAGS = sizeof(Default_Tags) / sizeof(char *);
                        static int selected_tags[64] = {0}; // Assuming a maximum of 64 tags
                        static int tree_state = 0;

                        // Tree tab for checkboxes
                        if (nk_tree_push(NK_Context, NK_TREE_TAB, "Select Tags Move to End", NK_MINIMIZED))
                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 20.0f, 4); // Adjust layout for checkboxes
                            for (int i = 0; i < NUM_TAGS; i++)
                            {
                                char tag_name[32];
                                snprintf(tag_name, sizeof(tag_name), "%s", Default_Tags[i]);
                                nk_checkbox_label(NK_Context, tag_name, &selected_tags[i]);
                            }
                            nk_tree_pop(NK_Context);
                        }

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3); // Layout for buttons
                        if (nk_button_label(NK_Context, "Move Selected"))
                        {
                            if (!sortMetaEdits)
                            {
                                u64 tag_mask = 0;
                                for (int i = 0; i < NUM_TAGS; i++)
                                {
                                    if (selected_tags[i])
                                    {
                                        tag_mask |= (1ULL << i);
                                    }
                                }
                                SortMapByMetaTags(tag_mask); // Pass the tag mask to the sorting function
                            }
                        }
                        if (nk_button_label(NK_Context, "Move All Tags"))
                        {
                            if (!sortMetaEdits)
                            {
                                u64 all_tags_mask = (1ULL << NUM_TAGS) - 1; // Create a mask with all bits set
                                SortMapByMetaTags(all_tags_mask);
                            }
                        }
                        if (nk_button_label(NK_Context, "Undo Move"))
                        {
                            while (sortMetaEdits)
                            {
                                UndoMapEdit();
                                sortMetaEdits--;
                            }
                        }
                    }

                    if (nk_tree_push(NK_Context, NK_TREE_TAB, "Colour Maps", NK_MINIMIZED))
                    {

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                        nk_checkbox_label(NK_Context, "Use Custom Order", &useCustomOrder);

                        showUserProfileScreen = nk_button_label(NK_Context, "Edit Order");

                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                        u32 currMap = Color_Maps->currMap;
                        
                        for (u32 i = 0; i < Color_Maps->nMaps; i++)
                        {
                            u32 index = useCustomOrder ? GetOrderedColorMapIndex(i) : i;
                            currMap = nk_option_label(NK_Context, Color_Map_Names[index], currMap == index) ? index : currMap;
                            nk_image(NK_Context, Color_Maps->mapPreviews[index]);
                        }

                        if (currMap != Color_Maps->currMap)
                        {
                            Color_Maps->currMap = currMap;
                            glActiveTexture(GL_TEXTURE1);
                            glBindTexture(GL_TEXTURE_BUFFER, Color_Maps->maps[Color_Maps->currMap]);
                            glActiveTexture(GL_TEXTURE0);
                        }

                        nk_tree_pop(NK_Context);
                    }

                    if (File_Loaded)
                    {
                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);

                            u32 nEdits = my_Min(Edits_Stack_Size, Map_Editor->nEdits);
                            char buff[128];
                            stbsp_snprintf((char *)buff, sizeof(buff), "Edits (%u)", nEdits);
                            
                            if (nk_tree_push(NK_Context, NK_TREE_TAB, (char *)buff, NK_MINIMIZED))
                            {
                                u32 editStackPtr = Map_Editor->editStackPtr == nEdits ? 0 : Map_Editor->editStackPtr;

                                f64 bpPerPixel = (f64)Total_Genome_Length / (f64)(Number_of_Textures_1D * Texture_Resolution);

                                ForLoop(nEdits)
                                {
                                    if (editStackPtr == nEdits)
                                    {
                                        editStackPtr = 0;
                                    }

                                    map_edit *edit = Map_Editor->edits + editStackPtr;

                                    u32 start = my_Min(edit->finalPix1, edit->finalPix2);
                                    u32 end = my_Max(edit->finalPix1, edit->finalPix2);
                                    u32 to = start ? start - 1 : (end < (Number_of_Pixels_1D - 1) ? end + 1 : end);

                                    u32 oldFrom = Map_State->contigRelCoords[start];
                                    u32 oldTo = Map_State->contigRelCoords[end];
                                    u32 *name1 = (Original_Contigs + Map_State->get_original_contig_id(start))->name;
                                    u32 *name2 = (Original_Contigs + Map_State->get_original_contig_id(end))->name;
                                    u32 *newName = (Original_Contigs + Map_State->get_original_contig_id(to))->name;
                                    
                                    stbsp_snprintf((char *)buff, sizeof(buff), "Edit %d:", index + 1);
                                    nk_label(NK_Context, (const char *)buff, NK_TEXT_LEFT);
                                    stbsp_snprintf((char *)buff, sizeof(buff), "       %s[%$.2fbp] to %s[%$.2fbp]",
                                            (char *)name1, (f64)oldFrom * bpPerPixel,
                                            (char *)name2, (f64)oldTo * bpPerPixel);
                                    nk_label(NK_Context, (const char *)buff, NK_TEXT_LEFT);

                                    nk_label(NK_Context, edit->finalPix1 > edit->finalPix2 ? (const char *)"       inverted and moved to" : 
                                            (const char *)"       moved to", NK_TEXT_LEFT);

                                    stbsp_snprintf((char *)buff, sizeof(buff), "       %s[%$.2fbp]",
                                            (char *)newName, (f64)Map_State->contigRelCoords[to] * bpPerPixel);
                                    nk_label(NK_Context, (const char *)buff, NK_TEXT_LEFT);

                                    ++editStackPtr;
                                }

                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                                if (nEdits && nk_button_label(NK_Context, "Undo")) UndoMapEdit();
                                if (nEdits && nk_button_label(NK_Context, "Copy to Clipboard")) CopyEditsToClipBoard(window);

                                nk_tree_pop(NK_Context);
                            }
                        }
                        
                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                            if (nk_tree_push(NK_Context, NK_TREE_TAB, "Waypoints", NK_MINIMIZED))
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);
                                char buff[4];

                                TraverseLinkedList(Waypoint_Editor->activeWaypoints.next, waypoint)
                                {
                                    stbsp_snprintf((char *)buff, sizeof(buff), "%d", node->index + 1);
                                    if (nk_button_label(NK_Context, (char *)buff))
                                    {
                                        Camera_Position.x = node->coords.x;
                                        Camera_Position.y = -node->coords.y;
                                        Camera_Position.z = node->z;
                                        ClampCamera();
                                    }
                                    
                                    if (nk_button_label(NK_Context, "Remove"))
                                    {
                                        waypoint *tmp = node->prev;
                                        RemoveWayPoint(node);
                                        node = tmp;
                                    }
                                }

                                nk_tree_pop(NK_Context);
                            }
                        }
                       
                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                            if (nk_tree_push(NK_Context, NK_TREE_TAB, "Scaffolds", NK_MINIMIZED))
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);

                                u32 scaffId = 0;
                                f32 pos = -0.5f;
                                f32 scaffLen = 0.0f;
                                u32 nSeq = 0;
                                ForLoop(Contigs->numberOfContigs)
                                {
                                    contig *cont = Contigs->contigs_arr + index;
                                    
                                    if (cont->scaffId != scaffId)
                                    {
                                        if (scaffId)
                                        {
                                            char buff[128];
                                            stbsp_snprintf((char *)buff, sizeof(buff), "Scaffold %u (%u)", scaffId, nSeq);
                                            if (nk_button_label(NK_Context, (char *)buff))
                                            {
                                                f32 p = pos - (0.5f * scaffLen);
                                                Camera_Position.x = p;
                                                Camera_Position.y = -p;
                                            }
                                        }
                                        
                                        scaffId = cont->scaffId;
                                        scaffLen = 0.0f;
                                        nSeq = 0;
                                    }
                                    
                                    ++nSeq;
                                    f32 d = (f32)((f64)cont->length / (f64)Number_of_Pixels_1D);
                                    pos += d;
                                    scaffLen += d;
                                }
                                if (scaffId)
                                {
                                    char buff[128];
                                    stbsp_snprintf((char *)buff, sizeof(buff), "Scaffold %u", scaffId);
                                    if (nk_button_label(NK_Context, (char *)buff))
                                    {
                                        f32 p = pos - (0.5f * scaffLen);
                                        Camera_Position.x = p;
                                        Camera_Position.y = -p;
                                    }
                                }

                                nk_tree_pop(NK_Context);
                            }
                        }

                        // Input Sequences
                        {   
                            std::string input_sequence_name = fmt::format("Input Sequences ({})", Number_of_Original_Contigs);
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                            if (nk_tree_push(NK_Context, NK_TREE_TAB, input_sequence_name.c_str(), NK_MINIMIZED))
                            {   
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 3);
                                nk_label(NK_Context, "Search: ", NK_TEXT_LEFT);
                                nk_edit_string_zero_terminated(NK_Context, NK_EDIT_FIELD, searchbuf, sizeof(searchbuf) - 1, nk_filter_default);
                                caseSensitive_search_sequences = nk_check_label(NK_Context, "Case Sensitive", caseSensitive_search_sequences) ? 1 : 0;
                                std::string searchbuf_str(searchbuf);

                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);

                                ForLoop(Number_of_Original_Contigs)
                                {
                                    original_contig *cont = Original_Contigs + index;

                                    std::string name_str((char *)cont->name);
                                    if (searchbuf_str.size()>0 && !caseSensitive_search_sequences)
                                    {
                                        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);
                                        std::transform(searchbuf_str.begin(), searchbuf_str.end(), searchbuf_str.begin(), ::tolower);
                                    }
                                    if (searchbuf_str.size()>0 && name_str.find(searchbuf_str) == std::string::npos) continue;

                                    char buff[128];
                                    stbsp_snprintf((char *)buff, sizeof(buff), "%s (%u)", (char *)cont->name, cont->nContigs);

                                    if (nk_tree_push_id(NK_Context, NK_TREE_TAB, (char *)buff, NK_MINIMIZED, index))
                                    {
                                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 2);

                                        ForLoop2(cont->nContigs)
                                        {   
                                            stbsp_snprintf((char *)buff, sizeof(buff), "%u", index2 + 1);
                                            if (nk_button_label(NK_Context, (char *)buff))
                                            {
                                                // f32 pos = (f32)((f64)cont->contigMapPixels[index2] / (f64)Number_of_Pixels_1D) - 0.5f;
                                                // Camera_Position.x = pos;
                                                // Camera_Position.y = -pos;

                                                f32 pos = (f32)((f64)cont->contigMapPixels[index2] / (f64)Number_of_Pixels_1D) - 0.5f;
                                                Camera_Position.x = pos;
                                                Camera_Position.y = -pos;
                                                Camera_Position.z = 1.0f;

                                                f32 contigSizeInPixels = (f32)cont->contigMapPixels[index2];
                                                f32 screenWidth = (f32)width;

                                                // f32 zoomLevel =(f32)(contigSizeInPixels / (screenWidth * index2));
                                                f32 zoomLevel = (f32)(contigSizeInPixels / (screenWidth));
                                                ZoomCamera(zoomLevel);

                                                // add a label / cover to show the fragments selected 
                                                Selected_Sequence_Cover_Countor.set(
                                                    index, 
                                                    index2,
                                                    GetTime(),
                                                    0.,
                                                    0.,
                                                    cont->contigMapPixels[index2]);

                                                Redisplay = 1;
                                            }

                                            if (nk_button_label(NK_Context, "Rebuild")) RebuildContig(cont->contigMapPixels[index2]);
                                        }
                                        
                                        nk_tree_pop(NK_Context);
                                    }
                                }

                                nk_tree_pop(NK_Context);
                            }
                        }

                        {
                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                            if (nk_tree_push(NK_Context, NK_TREE_TAB, "Tagged Sequences", NK_MINIMIZED))
                            {
                                ForLoop(ArrayCount(Meta_Data->tags))
                                {
                                    if (strlen((const char *)Meta_Data->tags[index]))
                                    {
                                        if (nk_tree_push_id(NK_Context, NK_TREE_TAB, (char *)Meta_Data->tags[index], NK_MINIMIZED, index))
                                        {
                                            f32 pos = -0.5f;
                                            ForLoop2(Contigs->numberOfContigs)
                                            {
                                                contig *cont = Contigs->contigs_arr + index2;
                                                f32 contLen = (f32)((f64)cont->length / (f64)Number_of_Pixels_1D);

                                                if (*cont->metaDataFlags & (1 << index))
                                                {
                                                    char buff[128];
                                                    u32 startCoord = cont->startCoord;
                                                    u32 endCoord = IsContigInverted(index2) ? (startCoord - cont->length + 1) : (startCoord + cont->length);

                                                    stbsp_snprintf((char *)buff, sizeof(buff), "%s [%$" PRIu64 "bp to %$" PRIu64 "bp]", (char *)((Original_Contigs + cont->get_original_contig_id())->name), (u64)((f64)startCoord / (f64)Number_of_Pixels_1D * (f64)Total_Genome_Length), (u64)((f64)(endCoord) / (f64)Number_of_Pixels_1D * (f64)Total_Genome_Length));
                                                    if (nk_button_label(NK_Context, (char *)buff))
                                                    {
                                                        f32 p = pos + (0.5f * contLen);
                                                        Camera_Position.x = p;
                                                        Camera_Position.y = -p;
                                                    }
                                                }

                                                pos += contLen;
                                            }
                                            
                                            nk_tree_pop(NK_Context);
                                        }
                                    }
                                }

                                nk_tree_pop(NK_Context);
                            }
                        }

                        {
                            if (Extensions.head)
                            {
                                nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                if (nk_tree_push(NK_Context, NK_TREE_TAB, "Extensions", NK_MINIMIZED))
                                {
                                    char buff[128];

                                    nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30.0f, 1);
                                    TraverseLinkedList(Extensions.head, extension_node)
                                    {
                                        switch (node->type)
                                        {
                                            case extension_graph:
                                                {
                                                    graph *gph = (graph *)node->extension;
                                                    
                                                    stbsp_snprintf(buff, sizeof(buff), "Graph: %s", (char *)gph->name);
                                                    
                                                    bounds = nk_widget_bounds(NK_Context);
                                                    gph->on = nk_check_label(NK_Context, buff, (s32)gph->on) ? 1 : 0;
                                                    if (nk_contextual_begin(NK_Context, 0, nk_vec2(Screen_Scale.x * 320, Screen_Scale.y * 340), bounds))
                                                    {
                                                        struct nk_colorf colour = gph->colour;

                                                        nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 330, 2);
                                                        nk_group_begin(NK_Context, "...1", 0);
                                                        {
                                                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                                                            nk_label(NK_Context, "Plot Colour", NK_TEXT_CENTERED);

                                                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 200, 1);
                                                            colour = nk_color_picker(NK_Context, colour, NK_RGBA);

                                                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                                                            if (nk_button_label(NK_Context, "Default")) colour = DefaultGraphColour;

                                                            gph->colour = colour;

                                                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                                                            gph->nameOn = nk_check_label(NK_Context, "Label", (s32)gph->nameOn) ? 1 : 0;

                                                            nk_group_end(NK_Context);
                                                        }
                                            
                                                        nk_group_begin(NK_Context, "...2", 0);
                                                        {
                                                            nk_layout_row_dynamic(NK_Context, Screen_Scale.y * 30, 1);
                                                            nk_label(NK_Context, "Plot Height", NK_TEXT_CENTERED);
                                                            nk_slider_float(NK_Context, -16.0f * DefaultGraphBase, &gph->base, 32.0f * DefaultGraphBase, 16.0f);
                                                            if (nk_button_label(NK_Context, "Default")) gph->base = DefaultGraphBase;

                                                            nk_label(NK_Context, "Plot Scale", NK_TEXT_CENTERED);
                                                            nk_slider_float(NK_Context, 0, &gph->scale, 8.0f * DefaultGraphScale, 0.005f);
                                                            if (nk_button_label(NK_Context, "Default")) gph->scale = DefaultGraphScale;

                                                            nk_label(NK_Context, "Line Width", NK_TEXT_CENTERED);
                                                            nk_slider_float(NK_Context, 0.1f, &gph->lineSize, 2.0f * DefaultGraphLineSize, 0.001f);
                                                            if (nk_button_label(NK_Context, "Default")) gph->lineSize = DefaultGraphLineSize;

                                                            nk_group_end(NK_Context);
                                                        }

                                                        nk_contextual_end(NK_Context);
                                                    }
                                                } break;
                                        }
                                    }
                                    nk_tree_pop(NK_Context);
                                }
                            }
                        }
                    }

                    nk_end(NK_Context);
                }

                if (FileBrowserRun("Load Map", &browser, NK_Context, (u32)showFileBrowser))
                {
                    if (!File_Loaded || !AreNullTerminatedStringsEqual(currFile, (u08 *)browser.file))
                    {
                        if (FILE *test = TestFile(browser.file))
                        {
                            fclose(test);
                            CopyNullTerminatedString((u08 *)browser.file, currFile);
                            Loading = 1;
                            currGroup1 = 0;
                            currGroup2 = 0;
                            currSelected1 = -1;
                            currSelected2 = -1;
                        }
                    }
                }

                if (currFileName)
                {
                    u08 state;
                    if ((state = FileBrowserRun("Save State", &saveBrowser, NK_Context, (u32)showSaveStateScreen, 1))) 
                    {
                        SaveState(headerHash, saveBrowser.file, state & 2);
                        FileBrowserReloadDirectoryContent(&saveBrowser, saveBrowser.directory);
                    }

                    if ((state = FileBrowserRun("Generate AGP", &saveAGPBrowser, NK_Context, (u32)showSaveAGPScreen, 2))) 
                    {
                        FenceIn(GenerateAGP(saveAGPBrowser.file, state & 2, state & 4, state & 8));
                        FileBrowserReloadDirectoryContent(&saveAGPBrowser, saveAGPBrowser.directory);
                    }

                    if (showClearCacheScreen)
                    {   
                        u32 window_height = 250;
                        u32 Window_Width = (u32)((f32)window_height * 1.618);
                        if (nk_begin(NK_Context, "Clear Cache", nk_rect(Screen_Scale.x * 600, Screen_Scale.y * 100, Screen_Scale.x * Window_Width, Screen_Scale.y * window_height),
                                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_TITLE))
                        {
                            nk_layout_row_dynamic(NK_Context, 80, 1);
                            nk_label(NK_Context, "Dangerous!!!", NK_TEXT_CENTERED);
                            nk_label(NK_Context, "Clear the cache and restore to", NK_TEXT_CENTERED);
                            nk_label(NK_Context, "initial state before curation?", NK_TEXT_CENTERED);

                            nk_layout_row_dynamic(NK_Context, 80, 2);
                            if (nk_button_label(NK_Context, "Yes"))
                            {
                                restore_initial_state();
                                // nk_popup_close(NK_Context);
                                showClearCacheScreen = 0;
                            }
                            if (nk_button_label(NK_Context, "No")) 
                            {
                                // nk_popup_close(NK_Context);
                                showClearCacheScreen = 0;
                            }

                            nk_end(NK_Context);
                        }
                    }

                    if (FileBrowserRun("Load State", &loadBrowser, NK_Context, (u32)showLoadStateScreen)) LoadState(headerHash, loadBrowser.file);

                    MetaTagsEditorRun(NK_Context, (u08)showMetaDataTagEditor);
                }

                AboutWindowRun(NK_Context, (u32)showAboutScreen);

                u08 state;
                if ((state = UserProfileEditorRun("User profile editor", &saveBrowser, NK_Context, (u32)showUserProfileScreen, 1)))
                {
                    if (showUserProfileScreen) 
                        fmt::print("[UserPofile]: showUserProfileScreen = {} \n", showUserProfileScreen);
                    UserSaveState("userprofile", state & 2, 0); // save the user profile settings
                    FileBrowserReloadDirectoryContent(&saveBrowser, saveBrowser.directory);
                }

                if (Deferred_Close_UI)
                {
                    UI_On = 0;
                    ++NK_Device->lastContextMemory[0];
                    Redisplay = 1;
                    Deferred_Close_UI = 0;
                }

                void *cmds = nk_buffer_memory(&NK_Context->memory);
                if (memcmp(cmds, NK_Device->lastContextMemory, NK_Context->memory.allocated))
                {
                    memcpy(NK_Device->lastContextMemory, cmds, NK_Context->memory.allocated);
                    Redisplay = 1;
                }
                else
                {
                    nk_clear(NK_Context);
                }
            }
        }
        else
        {
            glfwSetCursor(window, crossCursor);
        }

        if (!Redisplay) glfwWaitEvents();
    }

    if (currFileName) SaveState(headerHash);

    ThreadPoolWait(Thread_Pool);
    ThreadPoolDestroy(Thread_Pool);
    glfonsDelete(FontStash_Context);
    nk_font_atlas_clear(NK_Atlas);
    nk_free(NK_Context);
    nk_buffer_free(&NK_Device->cmds);
    glfwDestroyWindow(window);
    glfwTerminate();

    // free the memory allocated for the shader sources
    fprintf(stdout, "Memory freed for shader sources.\n");
    
    // do we need to free anything else? 
    // for example the allocated memory arena
    // and memory allocated  by new or malloc
    delete[] Contact_Matrix->vaos;
    delete[] Contact_Matrix->vbos;
    if (Grey_Out_Settings) 
    {
        delete Grey_Out_Settings; Grey_Out_Settings = nullptr;
    }
    if (user_profile_settings_ptr)
    {
        delete user_profile_settings_ptr; user_profile_settings_ptr = nullptr;
    }

    if (pixel_density_extension)
    {
        delete pixel_density_extension; pixel_density_extension = nullptr;
    }

    if (textures_array_ptr)
    {
        delete textures_array_ptr; textures_array_ptr = nullptr;
    }

    if (frag_cut_cal_ptr)
    {
        delete frag_cut_cal_ptr; frag_cut_cal_ptr = nullptr;
    }

    if (frag_sort_method) 
    {
        delete frag_sort_method; frag_sort_method = nullptr;
    }

    ResetMemoryArenaP(Loading_Arena);
    fprintf(stdout, "Memory freed for the arena.\n");

    EndMain;
}
