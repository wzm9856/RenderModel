#ifndef CAMERA_H
#define CAMERA_H
# define PI 3.14159265358979323846
#include <Eigen\Dense>
#include <vector>
typedef Eigen::Vector3f V3f;
typedef Eigen::Matrix4f M4f;

struct CameraPara {
    float width;
    float height;
    float f;
    float dx;
    float dy;
    float x0;
    float y0;
    float zNear = 100;
    float zFar = 10000;
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{

public:
    Camera(CameraPara Camerapara,
        V3f position = V3f(0.0f, 0.0f, 3.0f), V3f up = V3f(0.0f, 1.0f, 0.0f),
        float yaw = -90, float pitch = 0)
    {
        C = Camerapara;
        Position = position;
        WorldUp = up;
        Yaw = yaw * PI / 180;
        Pitch = pitch * PI / 180;
        updateCameraVectors();
        calculatePerspectiveMat();
        calculateViewMat();
    }

    // construct Camera from a viewmat
    Camera(M4f aviewMat) {
        viewMat = aviewMat;
        Right = viewMat.block(0, 0, 1, 3).transpose();
        Up = viewMat.block(1, 0, 1, 3).transpose();
        Front = -viewMat.block(2, 0, 1, 3).transpose();
        Eigen::Matrix3f A = viewMat.block(0, 0, 3, 3);
        A.block(0, 0, 2, 3) = -A.block(0, 0, 2, 3);
        Position = A.ldlt().solve(viewMat.block(0, 3, 3, 1));
    }

    // only use in oneStepRender
    Camera(int width, int height, M4f vm, M4f pm) {
        C.width = width; C.height = height;
        viewMat = vm; perspectiveMat = pm;
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    M4f getViewMatrix(){
        return viewMat;
    }

    M4f getPerspectiveMatrix() {
        return perspectiveMat;
    }

    V3f getPosition() {
        return Position;
    }

    V3f getUp() {
        return Up;
    }

    V3f getRight() {
        return Right;
    }

    V3f getFront() {
        return Front;
    }

    int getWidth() {
        return (int)C.width;
    }

    int getHeight() {
        return (int)C.height;
    }

    void setCameraPara(CameraPara CC) {
        C = CC;
        calculatePerspectiveMat();
    }

private:
    V3f Position;
    V3f Front = V3f(0.0f, 0.0f, -1.0f);
    V3f Up;
    V3f Right;
    V3f WorldUp;
    float Yaw;
    float Pitch;
    CameraPara C;
    M4f viewMat;
    M4f perspectiveMat;

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        Front.x() = cos(Yaw) * cos(Pitch);
        Front.y() = sin(Pitch);
        Front.z() = sin(Yaw) * cos(Pitch);
        Front.normalize();
        // also re-calculate the Right and Up vector
        Right = Front.cross(WorldUp);
        Right.normalize();
        Up = Right.cross(Front);
        Up.normalize();
    }

    void calculateViewMat() {
        viewMat = M4f::Zero();
        viewMat(0, 0) = Right.x();
        viewMat(0, 1) = Right.y();
        viewMat(0, 2) = Right.z();
        viewMat(1, 0) = Up.x();
        viewMat(1, 1) = Up.y();
        viewMat(1, 2) = Up.z();
        viewMat(2, 0) = -Front.x();
        viewMat(2, 1) = -Front.y();
        viewMat(2, 2) = -Front.z();
        viewMat(0, 3) = -Right.dot(Position);
        viewMat(1, 3) = -Up.dot(Position);
        viewMat(2, 3) = Front.dot(Position);
        viewMat(3, 3) = 1;
    }

    void calculatePerspectiveMat() {
        perspectiveMat = M4f::Zero();
        float r = (C.width - C.x0) * C.dx;
        float l = -C.x0 * C.dx;
        float t = (C.height - C.y0) * C.dy;
        float b = -C.y0 * C.dy;
        perspectiveMat(0, 0) = 2 * C.f / (r - l);
        perspectiveMat(1, 1) = 2 * C.f / (t - b);
        perspectiveMat(0, 2) = -(l + r) / (l - r);
        perspectiveMat(1, 2) = -(b + t) / (b - t);
        perspectiveMat(2, 2) = (C.zNear + C.zFar) / (C.zNear - C.zFar);
        perspectiveMat(2, 3) = 2 * C.zFar * C.zNear / (C.zNear - C.zFar);
        perspectiveMat(3, 2) = -1;
    }
};

#endif