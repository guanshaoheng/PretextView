#version 330
in float position;
uniform usamplerBuffer pixrearrangelookup;
uniform samplerBuffer yvalues;
uniform float ytop;
uniform float yscale;
void main()
{
    float x = position;
    float realx = texelFetch(pixrearrangelookup, int(x)).x;
    x /= textureSize(pixrearrangelookup);
    x -= 0.5;
    float y = texelFetch(yvalues, int(realx)).x;
    y *= yscale;
    y += ytop;

    gl_Position = vec4(x, y, 0.0, 1.0);
}