/*
 * M5 - Camera em primeira pessoa
 * Utiliza a classe Camera para encapsular posicao, orientacao e movimentacao.
 * Controles: WASD para andar, mouse para olhar ao redor, ESC para sair.
 */

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "LoadSimpleOBJ.h"
#include "Camera.h"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
int setupShader();
std::string getExecutableDir();

const GLuint WIDTH = 1000, HEIGHT = 1000;

// Camera em primeira pessoa
Camera globalCamera;
bool keys[1024] = {false};
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

const GLchar* vertexShaderSource = "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "layout (location = 1) in vec3 color;\n"
    "layout (location = 2) in vec2 texCoord;\n"
    "layout (location = 3) in vec3 normal;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "out vec3 vertexColor;\n"
    "out vec2 vertexTexCoord;\n"
    "out vec3 fragNormal;\n"
    "out vec3 fragPos;\n"
    "void main()\n"
    "{\n"
    "    vec4 worldPos = model * vec4(position, 1.0);\n"
    "    fragPos = worldPos.xyz;\n"
    "    gl_Position = projection * view * worldPos;\n"
    "    vertexColor = color;\n"
    "    vertexTexCoord = texCoord;\n"
    "    mat3 normalMatrix = transpose(inverse(mat3(model)));\n"
    "    fragNormal = normalize(normalMatrix * normal);\n"
    "}\n";

const GLchar* fragmentShaderSource = "#version 450\n"
    "in vec3 vertexColor;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 fragNormal;\n"
    "in vec3 fragPos;\n"
    "uniform sampler2D texture1;\n"
    "uniform vec3 ka;\n"
    "uniform vec3 kd;\n"
    "uniform vec3 ks;\n"
    "uniform float ns;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 L = normalize(lightPos - fragPos);\n"
    "    vec3 V = normalize(viewPos - fragPos);\n"
    "    vec3 R = reflect(-L, N);\n"
    "\n"
    "    vec3 texColor = texture(texture1, vertexTexCoord).rgb;\n"
    "\n"
    "    vec3 ambient = ka * 0.2;\n"
    "    vec3 diffuse = kd * texColor * max(dot(N, L), 0.0);\n"
    "    vec3 specular = ks * pow(max(dot(R, V), 0.0), ns);\n"
    "\n"
    "    color = vec4(ambient + diffuse + specular, 1.0);\n"
    "}\n";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M5 - Camera em primeira pessoa", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Captura o cursor do mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version supported " << version << std::endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    // Caminho base para os assets
    std::string basePath = getExecutableDir() + "/../Grau-A-M5-Camera/assets/";
    std::string objPath = basePath + "cube.obj";
    std::string mtlPath = basePath + "cube.mtl";

    int nVertices;
    GLuint objVAO = loadSimpleOBJ(objPath, nVertices);
    if (objVAO == (GLuint)-1)
    {
        std::cerr << "Falha ao carregar o modelo " << objPath << std::endl;
        return -1;
    }

    MaterialInfo mat = loadMaterialInfo(mtlPath);
    std::cout << "Material carregado:" << std::endl;
    std::cout << "  Ka = " << mat.ka.r << " " << mat.ka.g << " " << mat.ka.b << std::endl;
    std::cout << "  Kd = " << mat.kd.r << " " << mat.kd.g << " " << mat.kd.b << std::endl;
    std::cout << "  Ks = " << mat.ks.r << " " << mat.ks.g << " " << mat.ks.b << std::endl;
    std::cout << "  Ns = " << mat.ns << std::endl;
    std::cout << "  Textura: " << (mat.textureName.empty() ? "(nenhuma)" : mat.textureName) << std::endl;

    GLuint textureID = 0;
    if (!mat.textureName.empty())
    {
        std::string texturePath = basePath + mat.textureName;
        textureID = loadTexture(texturePath);
    }

    // Textura branca padrao caso nenhuma textura seja carregada
    if (textureID == 0)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        unsigned char white[4] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glUseProgram(shaderID);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");
    GLint kaLoc = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc = glGetUniformLocation(shaderID, "ks");
    GLint nsLoc = glGetUniformLocation(shaderID, "ns");
    GLint lightPosLoc = glGetUniformLocation(shaderID, "lightPos");
    GLint viewPosLoc = glGetUniformLocation(shaderID, "viewPos");

    // Luz fixa
    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
    glUniform3fv(kaLoc, 1, glm::value_ptr(mat.ka));
    glUniform3fv(kdLoc, 1, glm::value_ptr(mat.kd));
    glUniform3fv(ksLoc, 1, glm::value_ptr(mat.ks));
    glUniform1f(nsLoc, mat.ns);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

    glEnable(GL_DEPTH_TEST);

    // Aumenta velocidade de movimento para facilitar testes
    globalCamera.movementSpeed = 5.0f;

    // Posicoes de varios cubos para explorar a cena
    std::vector<glm::vec3> cubePositions;
    cubePositions.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
    cubePositions.push_back(glm::vec3(2.0f, 0.0f, -3.0f));
    cubePositions.push_back(glm::vec3(-2.0f, 0.0f, -2.0f));
    cubePositions.push_back(glm::vec3(0.0f, 1.5f, -5.0f));
    cubePositions.push_back(glm::vec3(3.0f, -0.5f, -1.0f));
    cubePositions.push_back(glm::vec3(-3.0f, 0.5f, -4.0f));

    std::cout << "\n=== Controles ===" << std::endl;
    std::cout << "W / S / A / D   : mover a camera" << std::endl;
    std::cout << "Mouse           : olhar ao redor" << std::endl;
    std::cout << "Scroll          : ajustar zoom (FOV)" << std::endl;
    std::cout << "R               : resetar camera" << std::endl;
    std::cout << "ESC             : sair" << std::endl;
    std::cout << "\nPosicao inicial da camera: (" << globalCamera.position.x << ", " << globalCamera.position.y << ", " << globalCamera.position.z << ")" << std::endl;
    std::cout << "Olhando para: (" << globalCamera.front.x << ", " << globalCamera.front.y << ", " << globalCamera.front.z << ")" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        // Calculo do deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // Processa movimento da camera
        bool moved = false;
        if (keys[GLFW_KEY_W])
        { globalCamera.processKeyboard(FORWARD, deltaTime); moved = true; }
        if (keys[GLFW_KEY_S])
        { globalCamera.processKeyboard(BACKWARD, deltaTime); moved = true; }
        if (keys[GLFW_KEY_A])
        { globalCamera.processKeyboard(LEFT, deltaTime); moved = true; }
        if (keys[GLFW_KEY_D])
        { globalCamera.processKeyboard(RIGHT, deltaTime); moved = true; }

        if (moved)
        {
            std::cout << "Camera pos: (" << globalCamera.position.x << ", " << globalCamera.position.y << ", " << globalCamera.position.z << ")" << std::endl;
        }

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

        // Matrizes de view e projecao atualizadas pela camera
        glm::mat4 view = globalCamera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(globalCamera.zoom),
                                                (float)WIDTH / (float)HEIGHT,
                                                0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(globalCamera.position));

        glBindVertexArray(objVAO);

        // Renderiza cada cubo em uma posicao diferente
        for (size_t i = 0; i < cubePositions.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);

            // Cubo central maior para ser referencia visual
            float scale = (i == 0) ? 1.5f : 0.8f;
            model = glm::scale(model, glm::vec3(scale));

            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, nVertices);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &objVAO);
    glDeleteTextures(1, &textureID);
    glDeleteProgram(shaderID);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        globalCamera = Camera();
        globalCamera.movementSpeed = 5.0f;
        std::cout << "Camera resetada para posicao inicial." << std::endl;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido: y vai de baixo para cima

    lastX = xpos;
    lastY = ypos;

    globalCamera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    globalCamera.processMouseScroll(yoffset);
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

std::string getExecutableDir()
{
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1)
    {
        path[len] = '\0';
        std::string p(path);
        return p.substr(0, p.find_last_of('/'));
    }
    return "";
}
