#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D bgTexture;

void main(){
    vec4 col = texture(bgTexture, TexCoords);
    FragColor = col;
} 