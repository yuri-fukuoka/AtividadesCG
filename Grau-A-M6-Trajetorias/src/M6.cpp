/*
 * M6 - Trajetorias
 * Cada objeto pode possuir uma trajetoria ciclica formada por pontos de controle.
 * - Adicionar pontos de controle a partir da posicao atual do objeto selecionado
 * - Mover objeto entre os pontos de forma ciclica (sem interpolacao cubica)
 * - Salvar e carregar trajetorias de arquivo
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "LoadSimpleOBJ.h"

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupLineShader();
std::string getExecutableDir();

void saveTrajectories(const std::string& filePath);
void loadTrajectories(const std::string& filePath);
void drawTrajectoryLine(const std::vector<glm::vec3>& points);

const GLuint WIDTH = 1000, HEIGHT = 1000;

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

const GLchar* lineVertexShaderSource = "#version 450\n"
    "layout (location = 0) in vec3 position;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = projection * view * vec4(position, 1.0);\n"
    "}\n";

const GLchar* lineFragmentShaderSource = "#version 450\n"
    "out vec4 color;\n"
    "void main()\n"
    "{\n"
    "    color = vec4(0.0, 1.0, 0.0, 1.0);\n"
    "}\n";

struct Object {
    glm::vec3 position;
    float scale;
    std::vector<glm::vec3> trajectory;
    size_t currentPoint;
    float t;
    bool animate;
};

std::vector<Object> objects;
int selectedObject = 0;

bool rotateX = false, rotateY = false, rotateZ = false;
bool showTrajectories = true;

glm::vec3 lightPos(2.0f, 2.0f, 2.0f);

GLuint lineShaderID = 0;
GLuint lineVAO = 0;
GLuint lineVBO = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
const float trajectorySpeed = 0.5f;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M6 - Trajetorias", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

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
    lineShaderID = setupLineShader();

    // Caminho base para os assets
    std::string basePath = getExecutableDir() + "/../Grau-A-M6-Trajetorias/assets/";
    std::string objPath = basePath + "cube.obj";
    std::string mtlPath = basePath + "cube.mtl";
    std::string trajectoryPath = basePath + "trajectories.txt";

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

    // Camera e projecao
    glm::vec3 cameraPos(0.0f, 0.0f, 8.0f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Parametros de luz e material
    glUniform3fv(kaLoc, 1, glm::value_ptr(mat.ka));
    glUniform3fv(kdLoc, 1, glm::value_ptr(mat.kd));
    glUniform3fv(ksLoc, 1, glm::value_ptr(mat.ks));
    glUniform1f(nsLoc, mat.ns);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

    glEnable(GL_DEPTH_TEST);

    // Instancia inicial
    objects.push_back({{0.0f, 0.0f, 0.0f}, 1.0f, {}, 0, 0.0f, true});

    // Carrega trajetorias se existirem
    loadTrajectories(trajectoryPath);

    std::cout << "\n=== Controles ===" << std::endl;
    std::cout << "X / Y / Z        : rotacionar no eixo" << std::endl;
    std::cout << "W / S            : mover no eixo Z" << std::endl;
    std::cout << "A / D            : mover no eixo X" << std::endl;
    std::cout << "I / J            : mover no eixo Y" << std::endl;
    std::cout << "[ / ]            : diminuir / aumentar escala" << std::endl;
    std::cout << "N                : adicionar objeto" << std::endl;
    std::cout << "B                : remover objeto selecionado" << std::endl;
    std::cout << "TAB              : selecionar proximo objeto" << std::endl;
    std::cout << "P                : adicionar ponto de controle a trajetoria" << std::endl;
    std::cout << "C                : limpar trajetoria do objeto selecionado" << std::endl;
    std::cout << "T                : ligar/desligar animacao do objeto" << std::endl;
    std::cout << "O                : ligar/desligar visualizacao das trajetorias" << std::endl;
    std::cout << "F5               : salvar trajetorias" << std::endl;
    std::cout << "F9               : carregar trajetorias" << std::endl;
    std::cout << "SETas            : mover luz no eixo X/Y" << std::endl;
    std::cout << "PGUP / PGDOWN    : mover luz no eixo Z" << std::endl;
    std::cout << "ESC              : sair" << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(2);
        glPointSize(5);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

        float angle = (GLfloat)glfwGetTime();

        glBindVertexArray(objVAO);

        for (int i = 0; i < (int)objects.size(); i++)
        {
            // Atualiza posicao pela trajetoria
            if (objects[i].animate && objects[i].trajectory.size() >= 2)
            {
                size_t next = (objects[i].currentPoint + 1) % objects[i].trajectory.size();
                glm::vec3 a = objects[i].trajectory[objects[i].currentPoint];
                glm::vec3 b = objects[i].trajectory[next];
                objects[i].position = a + (b - a) * objects[i].t;

                objects[i].t += trajectorySpeed * deltaTime;
                if (objects[i].t >= 1.0f)
                {
                    objects[i].t -= 1.0f;
                    objects[i].currentPoint = next;
                }
            }

            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, objects[i].position);
            model = glm::scale(model, glm::vec3(objects[i].scale));

            if (rotateX)
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (rotateY)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (rotateZ)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, nVertices);
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Desenha linhas das trajetorias
        if (showTrajectories)
        {
            glUseProgram(lineShaderID);
            glUniformMatrix4fv(glGetUniformLocation(lineShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(lineShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            for (int i = 0; i < (int)objects.size(); i++)
            {
                if (objects[i].trajectory.size() >= 2)
                    drawTrajectoryLine(objects[i].trajectory);
            }

            glUseProgram(shaderID);
        }

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &objVAO);
    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);
    glDeleteTextures(1, &textureID);
    glDeleteProgram(shaderID);
    glDeleteProgram(lineShaderID);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    const float step = 0.1f;
    const float scaleFactor = 0.1f;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    { rotateX = true;  rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    { rotateX = false; rotateY = true;  rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    { rotateX = false; rotateY = false; rotateZ = true;  }

    // Controle da luz
    const float lightStep = 0.2f;
    if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.x -= lightStep;
    if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.x += lightStep;
    if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.y += lightStep;
    if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.y -= lightStep;
    if (key == GLFW_KEY_PAGE_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.z += lightStep;
    if (key == GLFW_KEY_PAGE_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
        lightPos.z -= lightStep;

    if (action == GLFW_PRESS && (key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT ||
        key == GLFW_KEY_UP || key == GLFW_KEY_DOWN ||
        key == GLFW_KEY_PAGE_UP || key == GLFW_KEY_PAGE_DOWN))
    {
        std::cout << "Luz posicao: (" << lightPos.x << ", " << lightPos.y << ", " << lightPos.z << ")" << std::endl;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        showTrajectories = !showTrajectories;
        std::cout << "Visualizacao de trajetorias: " << (showTrajectories ? "ON" : "OFF") << std::endl;
    }

    if (objects.empty()) return;

    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.z -= step;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.z += step;

    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.x += step;
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.x -= step;

    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.y += step;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].position.y -= step;

    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].scale += scaleFactor;
    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        objects[selectedObject].scale = std::max(0.1f, objects[selectedObject].scale - scaleFactor);

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        float offset = objects.size() * 0.3f;
        objects.push_back({{offset, 0.0f, 0.0f}, 1.0f, {}, 0, 0.0f, true});
        selectedObject = objects.size() - 1;
        std::cout << "Objeto adicionado. Total: " << objects.size() << " | Selecionado: " << selectedObject << std::endl;
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS && objects.size() > 1)
    {
        objects.erase(objects.begin() + selectedObject);
        selectedObject = std::max(0, selectedObject - 1);
        std::cout << "Objeto removido. Total: " << objects.size() << " | Selecionado: " << selectedObject << std::endl;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedObject = (selectedObject + 1) % objects.size();
        std::cout << "Objeto selecionado: " << selectedObject << std::endl;
    }

    // Controles de trajetoria
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        objects[selectedObject].trajectory.push_back(objects[selectedObject].position);
        std::cout << "Ponto adicionado ao objeto " << selectedObject << ". Total: " << objects[selectedObject].trajectory.size() << std::endl;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        objects[selectedObject].trajectory.clear();
        objects[selectedObject].currentPoint = 0;
        objects[selectedObject].t = 0.0f;
        std::cout << "Trajetoria do objeto " << selectedObject << " limpa." << std::endl;
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        objects[selectedObject].animate = !objects[selectedObject].animate;
        std::cout << "Animacao do objeto " << selectedObject << ": " << (objects[selectedObject].animate ? "ON" : "OFF") << std::endl;
    }

    if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
        std::string basePath = getExecutableDir() + "/../Grau-A-M6-Trajetorias/assets/";
        saveTrajectories(basePath + "trajectories.txt");
    }

    if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
    {
        std::string basePath = getExecutableDir() + "/../Grau-A-M6-Trajetorias/assets/";
        loadTrajectories(basePath + "trajectories.txt");
    }
}

void saveTrajectories(const std::string& filePath)
{
    std::ofstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Erro ao salvar trajetorias em " << filePath << std::endl;
        return;
    }

    for (size_t i = 0; i < objects.size(); i++)
    {
        file << "object " << i << " " << objects[i].trajectory.size() << std::endl;
        for (size_t j = 0; j < objects[i].trajectory.size(); j++)
        {
            file << objects[i].trajectory[j].x << " "
                 << objects[i].trajectory[j].y << " "
                 << objects[i].trajectory[j].z << std::endl;
        }
    }

    file.close();
    std::cout << "Trajetorias salvas em " << filePath << std::endl;
}

void loadTrajectories(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cout << "Nenhum arquivo de trajetorias encontrado em " << filePath << std::endl;
        return;
    }

    objects.clear();
    selectedObject = 0;

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "object")
        {
            int idx;
            size_t count;
            ss >> idx >> count;
            objects.push_back({{0.0f, 0.0f, 0.0f}, 1.0f, {}, 0, 0.0f, true});

            for (size_t j = 0; j < count; j++)
            {
                std::getline(file, line);
                std::istringstream pointStream(line);
                glm::vec3 p;
                pointStream >> p.x >> p.y >> p.z;
                objects.back().trajectory.push_back(p);
            }
        }
    }

    file.close();

    if (objects.empty())
        objects.push_back({{0.0f, 0.0f, 0.0f}, 1.0f, {}, 0, 0.0f, true});

    std::cout << "Trajetorias carregadas de " << filePath << ". Objetos: " << objects.size() << std::endl;
}

void drawTrajectoryLine(const std::vector<glm::vec3>& points)
{
    if (points.size() < 2)
        return;

    std::vector<GLfloat> lineVertices;
    for (size_t i = 0; i < points.size(); i++)
    {
        glm::vec3 a = points[i];
        glm::vec3 b = points[(i + 1) % points.size()];
        lineVertices.push_back(a.x);
        lineVertices.push_back(a.y);
        lineVertices.push_back(a.z);
        lineVertices.push_back(b.x);
        lineVertices.push_back(b.y);
        lineVertices.push_back(b.z);
    }

    if (lineVAO == 0)
    {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
    }

    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(GLfloat), lineVertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINES, 0, lineVertices.size() / 3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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

int setupLineShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &lineVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::LINE_SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &lineFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::LINE_SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::LINE_SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
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
