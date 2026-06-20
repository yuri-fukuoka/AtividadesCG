/*
 * M2 - Cubo com transformações
 * Baseado no projeto Hello3D
 *
 * Implementações:
 * - Geometria de cubo (6 faces, 12 triângulos, cada face com cor diferente)
 * - Rotação nos eixos X, Y, Z (teclas x, y, z)
 * - Translação nos 3 eixos (W/S = z, A/D = x, I/J = y)
 * - Escala uniforme ([ para diminuir, ] para aumentar)
 * - Múltiplas instâncias do cubo (tecla N para adicionar, B para remover)
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = "#version 450\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 450\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

struct Cube {
    glm::vec3 position;
    float scale;
};

vector<Cube> cubes;
int selectedCube = 0;

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "M2 - Cubo 3D", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version  = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();
    GLuint VAO      = setupGeometry();

    glUseProgram(shaderID);
    GLint modelLoc = glGetUniformLocation(shaderID, "model");

    glEnable(GL_DEPTH_TEST);

    // Instância inicial
    cubes.push_back({{0.0f, 0.0f, 0.0f}, 1.0f});

    cout << "\n=== Controles ===" << endl;
    cout << "X / Y / Z   : rotacionar no eixo" << endl;
    cout << "W / S       : mover no eixo Z" << endl;
    cout << "A / D       : mover no eixo X" << endl;
    cout << "I / J       : mover no eixo Y" << endl;
    cout << "[ / ]       : diminuir / aumentar escala" << endl;
    cout << "N           : adicionar cubo" << endl;
    cout << "B           : remover cubo selecionado" << endl;
    cout << "TAB         : selecionar próximo cubo" << endl;
    cout << "ESC         : sair" << endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(2);
        glPointSize(5);

        float angle = (GLfloat)glfwGetTime();

        glBindVertexArray(VAO);

        for (int i = 0; i < (int)cubes.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);

            model = glm::translate(model, cubes[i].position);
            model = glm::scale(model, glm::vec3(cubes[i].scale));

            if (rotateX)
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            else if (rotateY)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            else if (rotateZ)
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    const float step  = 0.1f;
    const float scaleFactor = 0.1f;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Rotação
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    { rotateX = true;  rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    { rotateX = false; rotateY = true;  rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    { rotateX = false; rotateY = false; rotateZ = true;  }

    if (cubes.empty()) return;

    // Translação - eixo Z
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.z -= step;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.z += step;

    // Translação - eixo X
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.x += step;
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.x -= step;

    // Translação - eixo Y
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.y += step;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].position.y -= step;

    // Escala
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].scale += scaleFactor;
    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        cubes[selectedCube].scale = max(0.1f, cubes[selectedCube].scale - scaleFactor);

    // Instâncias
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        float offset = cubes.size() * 0.3f;
        cubes.push_back({{offset, 0.0f, 0.0f}, 1.0f});
        selectedCube = cubes.size() - 1;
        cout << "Cubo adicionado. Total: " << cubes.size() << " | Selecionado: " << selectedCube << endl;
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS && cubes.size() > 1)
    {
        cubes.erase(cubes.begin() + selectedCube);
        selectedCube = max(0, selectedCube - 1);
        cout << "Cubo removido. Total: " << cubes.size() << " | Selecionado: " << selectedCube << endl;
    }

    // Selecionar cubo
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedCube = (selectedCube + 1) % cubes.size();
        cout << "Cubo selecionado: " << selectedCube << endl;
    }
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
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int setupGeometry()
{
    // 6 faces, 2 triângulos cada = 12 triângulos = 36 vértices
    // Cada face tem uma cor diferente:
    // Frente   = vermelho
    // Trás     = verde
    // Esquerda = azul
    // Direita  = amarelo
    // Cima     = ciano
    // Baixo    = magenta
    GLfloat vertices[] = {
        // Frente (z = +0.5) - vermelho
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 0.0f, 0.0f,

        // Trás (z = -0.5) - verde
        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 0.0f,

        // Esquerda (x = -0.5) - azul
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,

        // Direita (x = +0.5) - amarelo
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,

        // Cima (y = +0.5) - ciano
        -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,

        // Baixo (y = -0.5) - magenta
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
    };

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}
