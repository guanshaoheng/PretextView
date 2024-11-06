#include "shaders.hpp"


GLchar* readShaderSource(char *shaderFile)
{
    FILE *fp = fopen(shaderFile, "r");

    if (fp == NULL)
    {
        fprintf(stderr, "Error: Could not open %s\n", shaderFile);
        assert(0);
    }
    else
    {
        #ifdef DEBUG
        fprintf(stdout, "Successfully opened %s\n", shaderFile);
        #endif  // DEBUG
    }

    fseek(fp, 0L, SEEK_END);
    long size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    char *buf = new GLchar[size + 1];
    fread(buf, 1, size, fp);

    buf[size] = '\0';
    fclose(fp);

    return buf;
}


void PrintShaderInfoLog(GLuint shader)
{
    s32 infoLogLen = 0;
    s32 charsWritten = 0;
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
    const GLchar *fragmentShaderSource, 
    const GLchar *vertexShaderSource, 
    const GLchar *geometryShaderSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
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
            char *pLinkInfoLog = new char[(s32)maxLength];
            glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, pLinkInfoLog);
            fprintf(stderr, "Failed to link shader: %s\n", pLinkInfoLog);
            // FreeLastPush(Working_Set);
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
