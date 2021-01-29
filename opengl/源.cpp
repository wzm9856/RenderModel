#define PI 3.14159265358979846
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define _CRT_SECURE_NO_WARNINGS
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <time.h>
#include <stb_image_write.h>
#include <Eigen/Dense>
#include <Eigen/Geometry> 
#include <iostream>
#include "shader.h"
#include "model.h"
#include "camera.h"
#include "render.h"
int main(){
    clock_t start, end;
    start = clock();

    M4f viewmat;
    //viewmat << 0.9691591262817383, -0.2464051693677902, 0.003896102774888277, 11.7416467666626,
    //    -0.003497301368042827, 0.002056083641946316, 0.9999918937683105, 1.788742303848267,
    //    -0.246411144733429, -0.9691647887229919, 0.001130918972194195, -1540.308349609375,
    //    0, 0, 0, 1;
    viewmat << -0.9965885877609253, -0.08214690536260605, -0.007943005301058292, -0.02309185080230236,
        -0.01408948749303818, 0.07451639324426651, 0.9971202611923218, 2.110600709915161,
        -0.08131846040487289, 0.9938305616378784, -0.0754195973277092, -4556.015625,
        0, 0, 0, 1;


    Camera ourCamera(viewmat);

    CameraPara C;
    C.width = 1920; C.height = 1440; C.dx = 5e-6; C.dy = 5e-6; C.f = 0.6125; C.x0 = C.width/2+10; C.y0 = C.height/2-10;
    ourCamera.setCameraPara(C);
    Render render(&ourCamera);
    render.InitWindow();
    Shader ourShader("objectShader.vs", "objectShader.fs");
    Model ourModel("./model/plane.obj");
    render.setSM(&ourShader, &ourModel);
    render.setbgImage("origin.png");

    render.setModelTransform(0, 0, 0, 0, 0, 0, 0.0257);
    render.draw();
    render.generateImage();
    render.getDepthInfo();

    //CameraPara C;
    //C.width = 1920; C.height = 1080; C.dx = 0.0001; C.dy = 0.0001; C.f = 1; C.x0 = C.width / 2; C.y0 = C.height / 2;
    //Camera ourCamera(C);
    //Render render(&ourCamera);
    //render.InitWindow();
    //Shader ourShader("test.vs", "test.fs");
    //render.setSM(&ourShader, NULL);
    //render.setModelTransform(0, 0, -400, PI / 4, PI / 4, PI / 4, 0.2);
    //oneStepRender(1920, 1080, "./model/engine_filled-n-37-n.obj", "test.vs", "test.fs", render.getModelMatrix(), ourCamera.getViewMatrix(), ourCamera.getPerspectiveMatrix());

    end = clock();
    cout << (end - start) / 1000 << endl;
}

