#version 330
layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4 matrix;
uniform float linewidth;

void main()
{
    vec3 start = gl_in[0].gl_Position.xyz;
    vec3 end = gl_in[1].gl_Position.xyz;
    vec3 lhs = cross(normalize(end-start), vec3(0.0, 0.0, -1.0));  // get a line prependicular to (end - start) on the xy plane

    lhs *= linewidth*0.0007;

    gl_Position = matrix * vec4(start+lhs, 1.0);  // offset the line to add width
    EmitVertex();
    gl_Position = matrix * vec4(start-lhs, 1.0);
    EmitVertex();
    gl_Position = matrix * vec4(end+lhs, 1.0);
    EmitVertex();
    gl_Position = matrix * vec4(end-lhs, 1.0);
    EmitVertex();
    EndPrimitive();
}