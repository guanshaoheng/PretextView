#version 330
in vec2 Texcoord;
in vec4 Color;
out vec4 outColor;
uniform sampler2D tex;
void main()
{
    outColor = texture(tex, Texcoord) * Color;
}