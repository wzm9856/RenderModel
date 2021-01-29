#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 Pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 perspective;

void main()
{
    Pos = aPos;
    gl_Position = perspective * view * model * vec4(aPos, 1.0);
}