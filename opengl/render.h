#pragma once

#include <GLFW/glfw3.h>
#include <Eigen\Dense>
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "helper_cuda.h"
#include <string>
typedef Eigen::Vector3f V3f;
typedef Eigen::Matrix4f M4f;

struct ModelTransformDesc {
    float tx = 0;
    float ty = 0;
    float tz = 0;
    float rx = 0;
    float ry = 0;
    float rz = 0;
    float scale = 1;
};

struct RenderDesc {
    bool isRenderBackGround = false;
    bool isRenderGrayImage = false;
    bool isMSAAEnable = false;
    Camera* camera = 0;
    Model* bodyModel = 0;
    Model* wingModel = 0;
    ModelTransformDesc* tranDesc = 0;
    std::string bgImagePath = "";
};

class Render {
public:
    Render(RenderDesc d);
    inline void setC(Camera* c);
    void setModelTransform(ModelTransformDesc* d);
    M4f getModelMatrix() { return modelMatrix; }
    void setbgImagePath(std::string imgPath);

    // 修改是否使用多重采样，同时设置好需要的frame buffer
    void setMSAAStatus(bool status);
    void draw();
    void generateImage(const char* filepath = "output.png");
    void getDepthInfo();
    void setbgRenderStatus(bool status);
    void setGrayRenderStatus(bool status);
    unsigned int getGrayTexture() { return grayTexture; }
    unsigned int getPosTexture() { return posTexture; }
private:
    Camera* camera;
    Shader* bodyShaderColor = NULL;
    Shader* bodyShaderGray = NULL;
    // 实际渲染时机身机翼使用相同shader文件，修改请到render构造函数的最后面
    Shader* wingShaderColor = NULL;
    // 实际渲染时机身机翼使用相同shader文件，修改请到render构造函数的最后面
    Shader* wingShaderGray = NULL;
    Shader* bgShaderColor = NULL;
    Shader* bgShaderGray = NULL;
    Model* bodyModel = NULL;
    Model* wingModel = NULL;
    std::string bgImagePath = "";
    M4f modelMatrix;
    int SCR_WIDTH;
    int SCR_HEIGHT;
    unsigned int framebuffer;
    unsigned int textureColorBufferMultiSampled;
    unsigned int posColorBufferMultiSampled;
    unsigned int rbo;
    unsigned int intermediateFBO;
    unsigned int grayTexture;
    unsigned int screenTexture;
    unsigned int posTexture;
    float* pPos = NULL;
    bool isRenderBackGround;
    bool isRenderGrayImage;
    bool isMSAAEnable;
    unsigned int bgVAO;
    unsigned int bgVBO;
    unsigned int bgTexture;
};

Render::Render(RenderDesc d){
    if (!d.camera || (!d.bodyModel && !d.wingModel)) {
        printf("RenderDesc is incomplete, program exiting...\n");
        exit(-1);
    }
    camera = d.camera;
    SCR_WIDTH = camera->getWidth();
    SCR_HEIGHT = camera->getHeight();
    isRenderBackGround = d.isRenderBackGround;
    isRenderGrayImage = d.isRenderGrayImage;
    isMSAAEnable = d.isMSAAEnable;
    bgImagePath = d.bgImagePath;

    stbi_set_flip_vertically_on_load(true);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // 先都准备好framebuffer和intermediateFBO，之后需要哪个就往哪里画
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // framebuffer的纹理缓冲
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
    // framebuffer的深度缓冲
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;

    // intermediateFBO
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (!isRenderGrayImage)glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

    glGenTextures(1, &grayTexture);
    glBindTexture(GL_TEXTURE_2D, grayTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (isRenderGrayImage)glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, grayTexture, 0);

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
        std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    
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
    glGenTextures(1, &bgTexture);

    bodyShaderColor = new Shader("objectShader.vs", "objectShader.fs");
    bodyShaderGray = new Shader("objectShader.vs", "objectShader_gray.fs");
    wingShaderColor = new Shader("objectShader.vs", "objectShader.fs");
    wingShaderGray = new Shader("objectShader.vs", "objectShader_gray.fs");
    bgShaderColor = new Shader("bgShader.vs", "bgShader.fs");
    bgShaderGray = new Shader("bgShader.vs", "bgShader_gray.fs");
    bodyModel = d.bodyModel;
    wingModel = d.wingModel;
    if(!bgImagePath.empty()) setbgImagePath(d.bgImagePath);
    setMSAAStatus(d.isMSAAEnable);
    setModelTransform(d.tranDesc);
}

void Render::setModelTransform(ModelTransformDesc* d) {
    float tx = d->tx; float ty = d->ty; float tz = d->tz;
    float rx = d->rx; float ry = d->ry; float rz = d->rz;
    float scale = d->scale;
    Eigen::Matrix3f rotation;
    rotation = Eigen::AngleAxisf(rx, V3f::UnitX())
        * Eigen::AngleAxisf(ry, V3f::UnitY())
        * Eigen::AngleAxisf(rz, V3f::UnitZ());
    V3f translation(tx, ty, tz);
    M4f modelM = M4f::Identity();
    modelM.block<3, 1>(0, 3) = translation;
    modelM.block<3, 3>(0, 0) = scale * rotation;
    modelMatrix = modelM;
}

void Render::setbgImagePath(std::string imagePath) {
    //加载背景图像
    int width, height, nchannels;
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &nchannels, 0);
    if (data == 0) {
        printf("Background image is not properly loaded\n");
    }
    glBindTexture(GL_TEXTURE_2D, bgTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

void Render::setMSAAStatus(bool status) {
    // 开启抗锯齿时只渲染颜色，进入framebuffer，之后转移到intermediaFBO中
    // 关闭抗锯齿时同时渲染颜色和坐标位置，直接渲染进intermediaFBO
    // 所以最后找东西都直接去 中间FBO 找
    isMSAAEnable = status;
    glClearColor(0, 0, 0, 0);
    if (isMSAAEnable) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
    else {
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
        const GLenum buffers[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, buffers);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    }
}

void Render::draw(){
    if (isMSAAEnable && isRenderGrayImage) {
        printf("render gray image while enable MSAA is not supported\n");
        return;
    }
    Shader* bodyShaderInUse = bodyShaderGray;
    Shader* wingShaderInUse = wingShaderGray;
    Shader* bgShaderInUse = bgShaderGray;
    if (!isRenderGrayImage) {
        bodyShaderInUse = bodyShaderColor;
        wingShaderInUse = wingShaderColor;
        bgShaderInUse = bgShaderColor;
    }
    if (isRenderBackGround) {
        bgShaderInUse->use();
        bgShaderInUse->setInt("bgTexture", 0);
        glBindVertexArray(bgVAO);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    M4f perspective = camera->getPerspectiveMatrix();
    M4f view = camera->getViewMatrix();
    if (bodyModel) {
        bodyShaderInUse->use();
        bodyShaderInUse->setMat4("perspective", perspective);
        bodyShaderInUse->setMat4("view", view);
        bodyShaderInUse->setMat4("model", modelMatrix);
        bodyModel->Draw(*bodyShaderInUse);
    }
    if (wingModel) {
        wingShaderInUse->use();
        wingShaderInUse->setMat4("perspective", perspective);
        wingShaderInUse->setMat4("view", view);
        wingShaderInUse->setMat4("model", modelMatrix);
        // wingShaderInUse->setFloat("G", G);
        wingModel->Draw(*wingShaderInUse);
    }

    // 如果开启抗锯齿，则需要将framebuffer中的内容转移到intermediateFBO中
    if (isMSAAEnable) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    }
}

void Render::generateImage(const char* outputpath) {
    if (!isRenderGrayImage) { //color image
        GLubyte* pPixelData = new GLubyte[(long)SCR_HEIGHT * SCR_WIDTH * 3];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pPixelData);
        stbi_flip_vertically_on_write(true);
        stbi_write_png(outputpath, SCR_WIDTH, SCR_HEIGHT, 3, pPixelData, 3 * SCR_WIDTH);
        delete[] pPixelData;
    }
    else {
        GLubyte* pPixelData = new GLubyte[(long)SCR_HEIGHT * SCR_WIDTH];
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grayTexture);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, pPixelData);
        stbi_flip_vertically_on_write(true);
        stbi_write_png(outputpath, SCR_WIDTH, SCR_HEIGHT, 1, pPixelData, SCR_WIDTH);
        delete[] pPixelData;
    }
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

void Render::setbgRenderStatus(bool status) {
    if (status && bgImagePath.empty()) {
        printf("use setbgImage() before activate bgRender\n");
        return;
    }
    isRenderBackGround = status;
}

void Render::setC(Camera* c) {
    if (c->getWidth() != SCR_WIDTH || c->getHeight() != SCR_HEIGHT) {
        throw "图像尺寸不同时需要创建不同的Render对象";
    }
    camera = c;
}

void Render::setGrayRenderStatus(bool status) {
    if (status == isRenderGrayImage) return;
    isRenderGrayImage = status;
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    if (isRenderGrayImage) {
        glBindTexture(GL_TEXTURE_2D, grayTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, grayTexture, 0);
    }
    else {
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
    }
    setMSAAStatus(isMSAAEnable);
}