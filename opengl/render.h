#pragma once

#include <GLFW/glfw3.h>
#include <Eigen\Dense>
#include "shader.h"
#include "camera.h"
#include "model.h"
typedef Eigen::Vector3f V3f;
typedef Eigen::Matrix4f M4f;

#define WZM_MSAA_ENABLE 1
#define WZM_MSAA_DISABLE 2

class Render {
public:
    Render(Camera* c);
    void setC(Camera* c) { 
        if (c->getWidth() != SCR_WIDTH || c->getHeight() != SCR_HEIGHT) {
            throw "图像尺寸不同时需要创建不同的Render对象";
        }
        camera = c; 
        if (bodyShader != NULL && wingShader != NULL) {
            M4f perspective = camera->getPerspectiveMatrix();
            M4f view = camera->getViewMatrix();
            bodyShader->setMat4("perspective", perspective);
            bodyShader->setMat4("view", view);
            wingShader->setMat4("perspective", perspective);
            wingShader->setMat4("view", view);
        }
    }
    void setSM(Shader* wingS, Shader* bodyS, Model* wingM, Model* bodyM) {
        wingShader = wingS; bodyShader = bodyS; bodyModel = bodyM; wingModel = wingM;
    }
    void setM(Model* bodyM, Model* wingM) { bodyModel = bodyM; wingModel = wingM; }

    void setModelTransform(float tX, float tY, float tZ, float rX, float rY, float rZ, float scale);
    M4f getModelMatrix() { return modelMatrix; }
    void setbgImage(const char* imagePath);
    void setFrameBuffer(int parameter);
    void draw(int parameter);
    void generateImage(const char* filepath = "output.png");
    void getDepthInfo();
private:
    Camera* camera;
    Shader* bodyShader = NULL;
    Shader* wingShader = NULL;
    Shader* bgshader;
    Model* bodyModel = NULL;
    Model* wingModel = NULL;
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
}

Render::Render(Camera* c){
    camera = c;
    SCR_WIDTH = camera->getWidth();
    SCR_HEIGHT = camera->getHeight();

    stbi_set_flip_vertically_on_load(true);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    //准备好framebuffer和intermediateFBO，之后需要哪个就往哪里画
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);


    // intermediateFBO
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

    glGenTextures(1, &posTexture);
    glBindTexture(GL_TEXTURE_2D, posTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, posTexture, 0);

    unsigned int rbo2;
    glGenRenderbuffers(1, &rbo2);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo2);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo2);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
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

void Render::setFrameBuffer(int parameter) {
    // 开启抗锯齿时只渲染颜色，进入framebuffer，之后转移到intermediaFBO中
    // 关闭抗锯齿时同时渲染颜色和坐标位置，直接渲染进intermediaFBO
    // 所以最后找东西都直接去 中间FBO 找
    if (parameter == WZM_MSAA_ENABLE) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
    else if (parameter == WZM_MSAA_DISABLE) {
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
        const GLenum buffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
    else {
        throw "传入参数好奇怪";
    }
}

void Render::draw(int parameter){
    if (dobg) {
        bgshader->use();
        bgshader->setInt("bgTexture", 0);
        glBindVertexArray(bgVAO);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    M4f perspective = camera->getPerspectiveMatrix();
    M4f view = camera->getViewMatrix();
    bodyShader->use();
    bodyShader->setMat4("perspective", perspective);
    bodyShader->setMat4("view", view);
    bodyShader->setMat4("model", modelMatrix);
    bodyModel->Draw(*bodyShader);

    wingShader->use();
    wingShader->setMat4("perspective", perspective);
    wingShader->setMat4("view", view);
    wingShader->setMat4("model", modelMatrix);
    wingModel->Draw(*bodyShader);

    if (parameter == WZM_MSAA_ENABLE) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    }
    else if (parameter == WZM_MSAA_DISABLE) {
        return;
    }
}

void Render::generateImage(const char* outputpath) {
    GLubyte* pPixelData = new GLubyte[(long)SCR_HEIGHT * SCR_WIDTH * 3];
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);
    stbi_flip_vertically_on_write(true);
    stbi_write_png(outputpath, SCR_WIDTH, SCR_HEIGHT, 3, pPixelData, 3 * SCR_WIDTH);
    delete[] pPixelData;
}

void Render::getDepthInfo() {
    pPos = new float[(long)SCR_HEIGHT * SCR_WIDTH * 3];
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, posTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pPos);
    //for (int i = 0; i < SCR_HEIGHT * SCR_WIDTH; i++) {
    //    if (pPos[3 * i] < -100) {
    //        cout << pPos[3 * i] << " " << pPos[3 * i + 1] << " " << pPos[3 * i + 2] << " " << endl;
    //    }
    //}
}
