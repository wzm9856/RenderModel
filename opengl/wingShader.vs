#version 330 core
#extension GL_NV_shader_buffer_load : enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 Pos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 perspective;
uniform float G;

float calculatePolynomial(in float coef[7],in int len,in float x) {
    float output = 0;
    for (int a = 0; a < len; a++) {
        output = output * x;
        output = output + coef[a];
    }
    return output;
}

void main()
{
    float x_mm, z_calib_mm_front, z_calib_mm_back, z_calib_mm_total, z_calib_inch, y_front, y_back;
    x_mm = aPos.x * 25.4;
    float coefficient_front[7] = float[](-6.279e-23, 1.302e-28, 4.245e-14, 2.873e-19, 2.453e-06, -5.177e-11, -54.852);
    float coefficient_back[7] = float[](-5.503e-23, -8.618e-21, 3.599e-14, 5.498e-12, 3.816e-06, -3.007e-04, -66.54);
    z_calib_mm_front = calculatePolynomial(coefficient_front, 7, x_mm);
    z_calib_mm_back = calculatePolynomial(coefficient_back, 7, x_mm);
    float linePoseFront[7] = float[]( -0.5192, 243.9 ,0,0,0,0,0);
    float linePoseBack[7] = float[]( -2.907e-16, 6.746e-16, 1.491e-10, -4.031e-10, -0.000323, 3.6e-05, -31.02 );
    y_front = calculatePolynomial(linePoseFront, 2, (aPos.x > 0 ? aPos.x : -aPos.x));
    y_back = calculatePolynomial(linePoseBack, 7, aPos.x);
    float ratio = (y_front - aPos.y) / (y_front - y_back);
    z_calib_mm_total = z_calib_mm_front * ratio + z_calib_mm_back * (1 - ratio);
    z_calib_inch = z_calib_mm_total / 25.4;

    Pos = aPos;
    Pos.z = Pos.z+z_calib_inch * (G / 2.5);
    gl_Position = perspective * view * model * vec4(Pos, 1.0);
}
