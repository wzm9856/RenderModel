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
    float x_mm = aPos.x*25.7;
    float z_calib_mm = -6E-23*pow(x_mm,6) + 1E-28*pow(x_mm,5) + 4E-14*pow(x_mm,4) + 3E-19*pow(x_mm,3) + 2E-06*pow(x_mm,2) - 5E-11*x_mm - 54.852;
    float z_calib_inch = z_calib_mm/25.7;
    Pos = aPos;
    Pos[2] = 0;
    gl_Position = perspective * view * model * vec4(Pos, 1.0);
}