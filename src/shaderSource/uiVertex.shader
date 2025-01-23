#version 330
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texcoord;
out vec2 Texcoord;
layout (location = 2) in vec4 color;
out vec4 Color;
uniform mat4 matrix;
void main()
{
    Texcoord = texcoord;
    Color = color;
    gl_Position = matrix * vec4(position, 0.0, 1.0);
}