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
    float x_mm = aPos.x*25.4;
    float coefficient_front[7] = 
    float z_calib_mm_front = 0;
    for(int i = 0; i<7; i++){
        z_calib_mm_front = z_calib_mm_front * x_mm;
        z_calib_mm_front = z_calib_mm_front + coefficient_front[i];
    }
    float z_calib_inch_front = z_calib_mm_front/254;

    float coefficient_back[7] = 
    float z_calib_mm_back = 0;
    for(int i = 0; i<7; i++){
        z_calib_mm_back = z_calib_mm_back * x_mm;
        z_calib_mm_back = z_calib_mm_back + coefficient_back[i];
    }
    float z_calib_inch_back = z_calib_mm_back/254;

    Pos = aPos;
    Pos.z = Pos.z+z_calib_inch_back;
    gl_Position = perspective * view * model * vec4(Pos, 1.0);
}