#include "utilsPretextView.h"



u32
texture_id_cal(u32 x, u32 y, u32 number_of_textures_1d)
{
    /*
        x is the row number
        y is the column number
        
        (Number_of_Textures_1D+Number_of_Textures_1D-x-1) * x  /2 + y - x
        = (2*Number_of_Textures_1D - x +  1 ) * x /2 + y - x + 1
        = (2*Number_of_Textures_1D - x -  1 ) * x /2 + y  + 1

        0,  1,  2,  3,  4,  5,  6,  7,  8,  9
        1, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        2, 11, 19, 20, 21, 22, 23, 24, 25, 26,
        3, 12, 21, 27, 28, 29, 30, 31, 32, 33,
        ...
        
    */
    if (x > y) std::swap(x, y);
    return (u32)(((( (number_of_textures_1d<<1 )- x - 1) * x)>>1)  + y)  ; 
}


void clearGL_Error() 
{
    while (glGetError() != GL_NO_ERROR){};
}


bool checkGL_Error(const char *msg)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) 
    {
        std::cerr << "OpenGL error: ["<< msg << "] 0x" << std::hex << err << "(" << std::dec << err << ")" << std::endl;
    }
    return err != GL_NO_ERROR;
}


std::string readShaderSource(char *shaderFile)
{   
    std::ifstream infile(shaderFile, std::ios::in | std::ios::binary);

    if (!infile)
    {
        fprintf(stderr, "Error: Could not open %s\n", shaderFile);
        assert(0);
    }

    std::stringstream buffer;
    buffer << infile.rdbuf();
    infile.close();
    return buffer.str();
}


void PrintShaderInfoLog(GLuint shader)
{
    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0)
    {
        infoLog = new GLchar[infoLogLen];
        glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        fprintf(stderr, "%s\n", infoLog);
        delete[] infoLog;
    }
}


GLuint CreateShader(
    const char *fragmentShaderSource, 
    const char *vertexShaderSource, 
    const char *geometryShaderSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); // make sure use this after the initialization of OpenGL
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint geometryShader = geometryShaderSource ? glCreateShader(GL_GEOMETRY_SHADER) : 0;

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    if (geometryShader) glShaderSource(geometryShader, 1, &geometryShaderSource, NULL);

    GLint compiled;

    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE)
    {
        PrintShaderInfoLog(vertexShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        exit(1);
    }

    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);

    if (compiled == GL_FALSE)
    {
        PrintShaderInfoLog(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        exit(1);
    }

    if (geometryShader)
    {
        glCompileShader(geometryShader);
        glGetShaderiv(geometryShader, GL_COMPILE_STATUS, &compiled);

        if (compiled == GL_FALSE)
        {
            PrintShaderInfoLog(geometryShader);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            glDeleteShader(geometryShader);
            exit(1);
        }
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    if (geometryShader) glAttachShader(shaderProgram, geometryShader);
    glLinkProgram(shaderProgram);

    GLint isLinked;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &isLinked);
    if(isLinked == GL_FALSE)
    {
        GLint maxLength;
        glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
        if(maxLength > 0)
        {
            char *pLinkInfoLog = new char[(int)maxLength];
            glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, pLinkInfoLog);
            fprintf(stderr, "Failed to link shader: %s\n", pLinkInfoLog);
            delete[] pLinkInfoLog;
        }

        glDetachShader(shaderProgram, vertexShader);
        glDetachShader(shaderProgram, fragmentShader);
        if (geometryShader) glDetachShader(shaderProgram, geometryShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        if (geometryShader) glDeleteShader(geometryShader);
        glDeleteProgram(shaderProgram);
        exit(1);
    }

    return(shaderProgram);
}


void push_nk_style(nk_context *ctx, nk_color normal, nk_color hover, nk_color active)
{
    nk_style_push_color(ctx, &(ctx->style.button.normal.data.color), normal);
    nk_style_push_color(ctx, &(ctx->style.button.hover.data.color),  hover);
    nk_style_push_color(ctx, &(ctx->style.button.active.data.color),  active);
    return ;
}


void pop_nk_style(nk_context *ctx, u32 num_color_pushed)
{
    for (u32 i = 0; i < num_color_pushed; i++)
    {
        nk_style_pop_color(ctx);
    }
}
