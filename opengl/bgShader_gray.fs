#version 330 core

in vec2 TexCoords;

uniform sampler2D bgTexture;

layout (location = 0) out float FragColor;
layout (location = 1) out vec3 Pos;

void main(){
    vec4 color = texture(bgTexture, TexCoords);
    FragColor = color.x*0.299 + color.y*0.587 + color.z*0.114;
    Pos = vec3(1e6, 1e6, 1e6);
} 