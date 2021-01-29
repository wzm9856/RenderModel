#pragma once

#include <GLFW/glfw3.h>
#include <Eigen\Dense>
#include "shader.h"
#include "camera.h"
#include "model.h"
typedef Eigen::Vector3f V3f;
typedef Eigen::Matrix4f M4f;
class Render {
public:
    Render(Camera* c) {
        camera = c;
        SCR_WIDTH = camera->getWidth();
        SCR_HEIGHT = camera->getHeight();
    }
    void setC(Camera* c) { 
        if (c->getWidth() != SCR_WIDTH || c->getHeight() != SCR_HEIGHT) {
            throw "图像尺寸不同时需要创建不同的Render对象";
        }
        camera = c; 
        if (shader != NULL) {
            M4f perspective = camera->getPerspectiveMatrix();
            M4f view = camera->getViewMatrix();
            shader->setMat4("perspective", perspective);
            shader->setMat4("view", view);
        }
    }
    void setSM(Shader* s, Model* m) { 
        shader = s; model = m; 
        shader->use(); 
        M4f perspective = camera->getPerspectiveMatrix();
        M4f view = camera->getViewMatrix();
        shader->setMat4("perspective", perspective);
        shader->setMat4("view", view);
    }
    void setM(Model* m) { model = m; }

    // need radian as rotation input
    void setModelTransform(float tX, float tY, float tZ, float rX, float rY, float rZ, float scale);
    M4f getModelMatrix() { return modelMatrix; }
    void getDepthInfo();
    void setbgImage(const char* imagePath);
    // need image height and width to initalize window object, so Camera object is needed. 
    // parameters other than image size can be changed later.
    GLFWwindow* InitWindow();

    void draw();
    void generateImage(const char* filepath = "output.png");
private:
    Camera* camera;
    Shader* shader = NULL;
    Shader* bgshader;
    Model* model = NULL;
    M4f modelMatrix;
    int SCR_WIDTH;
    int SCR_HEIGHT;
    unsigned int framebuffer;
    unsigned int textureColorBufferMultiSampled;
    unsigned int posColorBufferMultiSampled;
    unsigned int rbo;
    unsigned int intermediateFBO;
    unsigned int screenTexture;
    unsigned int posTexture;
    float* pPos = NULL;
    bool dobg = false;
    unsigned int bgVAO;
    unsigned int bgVBO;
    unsigned int bgTexture;
};

void Render::setModelTransform(float tX, float tY, float tZ, float rX, float rY, float rZ, float scale) {
    Eigen::Matrix3f rotation;
    rotation = Eigen::AngleAxisf(rX, V3f::UnitX())
        * Eigen::AngleAxisf(rY, V3f::UnitY())
        * Eigen::AngleAxisf(rZ, V3f::UnitZ());
    V3f translation(tX, tY, tZ);
    M4f modelM = M4f::Identity();
    modelM.block<3, 1>(0, 3) = translation;
    modelM.block<3, 3>(0, 0) = scale * rotation;
    modelMatrix = modelM;
    shader->setMat4("model", modelM);
}

GLFWwindow* Render::InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Plane", NULL, NULL);
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
    stbi_set_flip_vertically_on_load(true);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // create a multisampled color attachment texture
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

    glGenTextures(1, &posColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, posColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, posColorBufferMultiSampled, 0);

    // create a (also multisampled) renderbuffer object for depth and stencil attachments
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 把多重采样变为单重采样的帧缓冲
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    // 用于储存颜色的纹理
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

    // 用于储存模型坐标的纹理
    glGenTextures(1, &posTexture);
    glBindTexture(GL_TEXTURE_2D, posTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, posTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    return window;
}

void Render::setbgImage(const char* imagePath) {
    bgshader = new Shader("bgShader.vs", "bgShader.fs");
    dobg = true;

    // 设置背景数组缓存，坐标z值为远平面z值以保证渲染时背景在最后面
    float bgVertices[] = { // 位置坐标 纹理坐标
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bgVertices), &bgVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    //加载背景图像
    glGenTextures(1, &bgTexture);
    int width, height, nchannels;
    unsigned char* data = stbi_load(imagePath, &width, &height, &nchannels, 0);
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

void Render::draw() {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_DEPTH_BUFFER_BIT);
    const float color1[] = { 0.3,0.6,0.9,1.0 };
    glClearBufferfv(GL_COLOR, 0, color1);
    // 先画背景（如果有）
    if (dobg) { 
        bgshader->use();
        bgshader->setInt("bgTexture", 0);
        glBindVertexArray(bgVAO);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    const GLenum buffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, buffers);
    const float color2[] = { 0.9, 0.6, 0.3, 1 };
    glClearBufferfv(GL_COLOR, 1, color2);       // 模型坐标信息不需要背景，在画模型之前要clear一下背景
    model->Draw(*shader);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
}

void Render::generateImage(const char* outputpath) {
    GLubyte* pPixelData = new GLubyte[SCR_HEIGHT * SCR_WIDTH * 3];
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);
    //glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);

    stbi_flip_vertically_on_write(true);
    stbi_write_png(outputpath, SCR_WIDTH, SCR_HEIGHT, 3, pPixelData, 3 * SCR_WIDTH);
    delete pPixelData;
}

void Render::getDepthInfo() {
    pPos = new float[SCR_HEIGHT * SCR_WIDTH * 3];
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, posTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pPos);
    for (int i = 0; i < SCR_WIDTH * SCR_HEIGHT; i++) {
        if ((pPos[3 * i] - 0.9) < 1e6 && (pPos[3 * i + 1] - 0.6) < 1e6 && (pPos[3 * i + 2] - 0.3) < 1e6) {
            pPos[3 * i] = 1e6;
            pPos[3 * i + 1] = 1e6;
            pPos[3 * i + 2] = 1e6;
        }
    }
}

void oneStepRender(int SCR_WIDTH, int SCR_HEIGHT, const char* modelFilepath, const char* vsFilepath, const char* fsFilepath, M4f modelMat, M4f viewMat, M4f perspectiveMat, const char* outputpath = "outputOnestep.png") {
    Camera camera(SCR_WIDTH, SCR_HEIGHT, viewMat, perspectiveMat);
    Render render(&camera);
    render.InitWindow();
    Shader shader(vsFilepath, fsFilepath);
    Model model(modelFilepath);
    render.setSM(&shader, &model);
    shader.setMat4("model", modelMat);
    render.draw();
    render.generateImage(outputpath);
}