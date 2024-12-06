#version 330
in vec2 position;  // x, y
in vec3 texcoord;  // s, t, u
out vec3 Texcoord;
uniform mat4 matrix;

void main()
{
    Texcoord = texcoord;
    gl_Position = matrix * vec4(position, 0.0, 1.0);
}