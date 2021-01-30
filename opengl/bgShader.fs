#version 330 core

in vec2 TexCoords;

uniform sampler2D bgTexture;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 Pos;

void main(){
    vec4 col = texture(bgTexture, TexCoords);
    FragColor = col;
    Pos = vec3(1e6, 1e6, 1e6);
} 