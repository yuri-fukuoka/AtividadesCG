#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

// Direcoes de movimento da camera
enum CameraMovement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Classe que encapsula uma camera em primeira pessoa usando angulos de Euler.
class Camera
{
public:
    // Atributos da camera
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float zoom;


    // Construtor com valores padrao
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
           float yaw = -90.0f,
           float pitch = 0.0f);

    // Retorna a matriz view (lookAt) a partir dos vetores da camera
    glm::mat4 getViewMatrix();

    // Processa input do teclado (WASD)
    void processKeyboard(CameraMovement direction, float deltaTime);

    // Processa movimento do mouse
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Processa scroll do mouse para ajustar zoom/FOV
    void processMouseScroll(float yoffset);

private:
    // Recalcula os vetores front, right e up a partir de yaw e pitch
    void updateCameraVectors();
};

#endif
