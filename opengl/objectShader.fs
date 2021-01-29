#version 330 core

in vec3 Pos;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 Pos1;

uniform vec4 color;

void main()
{    
    FragColor = color;
    Pos1 = Pos;
}