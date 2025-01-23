#version 330 core
in vec2 TexCoord;
out vec4 FragColor; 

uniform sampler2DArray texArray; // 0
uniform int layer;  

void main() {

    FragColor = vec4(texture(texArray, vec3(TexCoord, layer)).r, 0., 0., 1.);
    
}