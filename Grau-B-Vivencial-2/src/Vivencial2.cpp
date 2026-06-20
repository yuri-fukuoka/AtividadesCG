/*
 * Vivencial 2 - Iluminacao de 3 Pontos
 *
 * Implementa a tecnica de iluminacao de 3 pontos posicionada automaticamente
 * a partir da posicao e escala do objeto principal da cena:
 *
 *   Key light   - luz principal, intensa, lateral-frontal (esquerda, acima, frente)
 *   Fill light  - luz de preenchimento, suave, lado oposto a key light
 *   Back light  - luz de fundo, atras e acima do objeto
 *
 * Modelo Phong com 3 fontes de luz acumuladas no fragment shader.
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
std::string getExecutableDir();

const GLuint WIDTH = 1000, HEIGHT = 1000;

// ─── Vertex Shader ──────────────────────────────────────────────────────────
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

// ─── Fragment Shader - 3 luzes Phong ────────────────────────────────────────
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
    "uniform vec3 viewPos;\n"
    // Tres luzes: posicao e intensidade (cor)
    "uniform vec3 keyLightPos;\n"
    "uniform vec3 keyLightColor;\n"
    "uniform vec3 fillLightPos;\n"
    "uniform vec3 fillLightColor;\n"
    "uniform vec3 backLightPos;\n"
    "uniform vec3 backLightColor;\n"
    "out vec4 color;\n"
    "\n"
    "vec3 calcLight(vec3 lightPos, vec3 lightColor, vec3 N, vec3 V, vec3 texColor)\n"
    "{\n"
    "    vec3 L = normalize(lightPos - fragPos);\n"
    "    vec3 R = reflect(-L, N);\n"
    "    vec3 diffuse  = kd * texColor * lightColor * max(dot(N, L), 0.0);\n"
    "    vec3 specular = ks * lightColor * pow(max(dot(R, V), 0.0), ns);\n"
    "    return diffuse + specular;\n"
    "}\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec3 N = normalize(fragNormal);\n"
    "    vec3 V = normalize(viewPos - fragPos);\n"
    "    vec3 texColor = texture(texture1, vertexTexCoord).rgb;\n"
    "    vec3 ambient  = ka * 0.1;\n"
    "    vec3 lighting = calcLight(keyLightPos,  keyLightColor,  N, V, texColor)\n"
    "                  + calcLight(fillLightPos, fillLightColor, N, V, texColor)\n"
    "                  + calcLight(backLightPos, backLightColor, N, V, texColor);\n"
    "    color = vec4(ambient + lighting, 1.0);\n"
    "}\n";

// ─── Estado global ───────────────────────────────────────────────────────────
struct Object {
    glm::vec3 position;
    float scale;
};

std::vector<Object> objects;
int selectedObject = 0;
bool rotateX = false, rotateY = false, rotateZ = false;

// ─── Calcula as 3 posicoes de luz relativas ao objeto principal ──────────────
//
//  key light  : lateral esquerda, acima, na frente
//  fill light : lateral direita, levemente abaixo, na frente (intensidade 50%)
//  back light : atras e acima (intensidade 70%)
//
//  O raio de posicionamento e proporcional a escala do objeto.
//
void computeLightPositions(const Object& obj,
                           glm::vec3& keyPos,
                           glm::vec3& fillPos,
                           glm::vec3& backPos)
{
    float r = obj.scale * 3.0f;  // distancia das luzes em funcao da escala
    glm::vec3 c = obj.position;  // centro do objeto

    // Key light: 45 graus a esquerda, 45 graus acima, ligeiramente a frente
    keyPos  = c + glm::vec3(-r * 0.707f,  r * 0.707f,  r * 0.5f);

    // Fill light: lado direito, levemente abaixo, mesma profundidade que key
    fillPos = c + glm::vec3( r * 0.707f,  r * 0.2f,    r * 0.5f);

    // Back light: atras e acima (separa o objeto do fundo)
    backPos = c + glm::vec3( 0.0f,        r * 0.5f,   -r);
}

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT,
        "Vivencial 2 - Iluminacao de 3 Pontos", nullptr, nullptr);
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
    std::cout << "OpenGL version: " << version << std::endl;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    // Caminhos
    std::string exeDir  = getExecutableDir();
    std::string objPath = exeDir + "/../Assets/Pallone/Ball OBJ.obj";

    int nVertices;
    GLuint objVAO = loadSimpleOBJ(objPath, nVertices);
    if (objVAO == (GLuint)-1)
    {
        std::cerr << "Falha ao carregar o modelo: " << objPath << std::endl;
        return -1;
    }

    // Material padrao (bola nao tem MTL)
    MaterialInfo mat;
    mat.ka = glm::vec3(0.1f);
    mat.kd = glm::vec3(0.8f, 0.8f, 0.85f);
    mat.ks = glm::vec3(1.0f);
    mat.ns = 64.0f;

    // Textura branca padrao
    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(shaderID);

    GLint modelLoc       = glGetUniformLocation(shaderID, "model");
    GLint viewLoc        = glGetUniformLocation(shaderID, "view");
    GLint projLoc        = glGetUniformLocation(shaderID, "projection");
    GLint kaLoc          = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc          = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc          = glGetUniformLocation(shaderID, "ks");
    GLint nsLoc          = glGetUniformLocation(shaderID, "ns");
    GLint viewPosLoc     = glGetUniformLocation(shaderID, "viewPos");
    GLint keyPosLoc      = glGetUniformLocation(shaderID, "keyLightPos");
    GLint keyColorLoc    = glGetUniformLocation(shaderID, "keyLightColor");
    GLint fillPosLoc     = glGetUniformLocation(shaderID, "fillLightPos");
    GLint fillColorLoc   = glGetUniformLocation(shaderID, "fillLightColor");
    GLint backPosLoc     = glGetUniformLocation(shaderID, "backLightPos");
    GLint backColorLoc   = glGetUniformLocation(shaderID, "backLightColor");

    // Camera
    glm::vec3 cameraPos(0.0f, 0.0f, 5.0f);
    glm::mat4 view = glm::lookAt(cameraPos,
                                  glm::vec3(0.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                             (float)WIDTH / (float)HEIGHT,
                                             0.1f, 100.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

    glUniform3fv(kaLoc, 1, glm::value_ptr(mat.ka));
    glUniform3fv(kdLoc, 1, glm::value_ptr(mat.kd));
    glUniform3fv(ksLoc, 1, glm::value_ptr(mat.ks));
    glUniform1f(nsLoc, mat.ns);

    // Cores das luzes
    glm::vec3 keyColor  (1.0f,  1.0f,  0.95f);  // key:  branca quente, intensidade plena
    glm::vec3 fillColor (0.4f,  0.4f,  0.5f);   // fill: azulada, 40% de intensidade
    glm::vec3 backColor (0.6f,  0.5f,  0.8f);   // back: roxo claro, 60% de intensidade

    glUniform3fv(keyColorLoc,  1, glm::value_ptr(keyColor));
    glUniform3fv(fillColorLoc, 1, glm::value_ptr(fillColor));
    glUniform3fv(backColorLoc, 1, glm::value_ptr(backColor));

    glEnable(GL_DEPTH_TEST);

    objects.push_back({{0.0f, 0.0f, 0.0f}, 1.0f});

    std::cout << "\n=== Controles ===" << std::endl;
    std::cout << "X / Y / Z     : rotacionar no eixo" << std::endl;
    std::cout << "W / S         : mover no eixo Z" << std::endl;
    std::cout << "A / D         : mover no eixo X" << std::endl;
    std::cout << "I / J         : mover no eixo Y" << std::endl;
    std::cout << "[ / ]         : diminuir / aumentar escala" << std::endl;
    std::cout << "N             : adicionar objeto" << std::endl;
    std::cout << "B             : remover objeto selecionado" << std::endl;
    std::cout << "TAB           : selecionar proximo objeto" << std::endl;
    std::cout << "ESC           : sair" << std::endl;
    std::cout << "\nAs 3 luzes sao reposicionadas automaticamente." << std::endl;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0);

        // Recalcula as posicoes das luzes com base no objeto principal (objects[0])
        glm::vec3 keyPos, fillPos, backPos;
        computeLightPositions(objects[0], keyPos, fillPos, backPos);

        glUniform3fv(keyPosLoc,  1, glm::value_ptr(keyPos));
        glUniform3fv(fillPosLoc, 1, glm::value_ptr(fillPos));
        glUniform3fv(backPosLoc, 1, glm::value_ptr(backPos));

        float angle = (GLfloat)glfwGetTime();

        glBindVertexArray(objVAO);

        for (int i = 0; i < (int)objects.size(); i++)
        {
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
        objects.push_back({{offset, 0.0f, 0.0f}, 1.0f});
        selectedObject = objects.size() - 1;
        std::cout << "Objeto adicionado. Total: " << objects.size()
                  << " | Selecionado: " << selectedObject << std::endl;
    }

    if (key == GLFW_KEY_B && action == GLFW_PRESS && objects.size() > 1)
    {
        objects.erase(objects.begin() + selectedObject);
        selectedObject = std::max(0, selectedObject - 1);
        std::cout << "Objeto removido. Total: " << objects.size()
                  << " | Selecionado: " << selectedObject << std::endl;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedObject = (selectedObject + 1) % objects.size();
        std::cout << "Objeto selecionado: " << selectedObject << std::endl;
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
