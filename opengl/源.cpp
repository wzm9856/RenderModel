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

void custom_glfwInit(CameraPara& C) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(C.width, C.height, "Plane", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
}

int main(){
    clock_t start, end;
    start = clock();

    CameraPara C;
    C.width = 1920; C.height = 1440; C.dx = 5e-6; C.dy = 5e-6; C.f = 0.6125; C.x0 = C.width / 2 + 10; C.y0 = C.height / 2 - 10;
    custom_glfwInit(C);
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

    ourCamera.setCameraPara(C);
    Render render(&ourCamera);
    Shader bodyShader("objectShader.vs", "objectShader.fs");
    Shader wingShader("wingShader.vs", "objectShader.fs");
    float coefficient[] = { -6.279e-23, 1.302e-28, 4.245e-14, 2.873e-19, 2.453e-06, -5.177e-11, -54.852 };

    Model wingModel("./model/wing.obj", 7, coefficient);
    wingModel.saveModel();
    Model bodyModel("./model/body.obj");
    //wingModel.wingTransform(coefficient, 7);

    render.setSM(&wingShader, &bodyShader, &wingModel, &bodyModel);
    render.setbgImage("origin2.png");
    render.setFrameBuffer(WZM_MSAA_DISABLE);

    render.setModelTransform(0, 0, 0, 0, 0, 0, 0.0254);
    render.draw(WZM_MSAA_DISABLE);
    render.generateImage();
    render.getDepthInfo();

    end = clock();
    cout << (end - start) / 1000 << endl;
}

