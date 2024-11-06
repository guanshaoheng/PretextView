#ifndef SHADERS_HPP
#define SHADERS_HPP

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cassert>


typedef int s32;


GLchar* readShaderSource(char *shaderFile);


void PrintShaderInfoLog(GLuint shader);


GLuint CreateShader(
    const GLchar *fragmentShaderSource, 
    const GLchar *vertexShaderSource, 
    const GLchar *geometryShaderSource=0);


#endif // SHADERS_HPP