/*
 * Projeto Final - Cena de Futebol
 *
 * Cena com bola e goleira, iluminacao Phong, camera em primeira pessoa,
 * selecao de objetos e trajetoria da bola por curva de Bezier cubica.
 *
 * Controles:
 *   WASD          : mover camera
 *   Mouse         : orientar camera
 *   TAB           : selecionar objeto (bola / goleira)
 *   Setas         : mover objeto selecionado (X/Z)
 *   I / K         : mover objeto selecionado (Y)
 *   R             : rotacionar objeto (Y automatico)
 *   [ / ]         : diminuir / aumentar escala
 *   ESPACO        : iniciar/parar trajetoria da bola
 *   J/L/U/O/P/N   : mover luz (X/Y/Z)
 *   ESC           : sair
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <unistd.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "LoadSimpleOBJ.h"

// ─── Utilitario ──────────────────────────────────────────────────────────────
static std::string getExeDir()
{
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) { buf[len] = '\0'; std::string s(buf); return s.substr(0, s.find_last_of('/')); }
    return ".";
}

// ─── Dimensoes da janela ──────────────────────────────────────────────────────
const int SCR_W = 1280;
const int SCR_H =  720;

// ─── Camera em primeira pessoa ────────────────────────────────────────────────
struct Camera {
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    float yaw, pitch;
    float speed, sensitivity, fov;

    Camera() : pos(0,1.5f,8), yaw(-90.0f), pitch(-5.0f),
               speed(5.0f), sensitivity(0.02f), fov(45.0f)
    {
        up = glm::vec3(0,1,0);
        updateVectors();
    }

    void updateVectors()
    {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        right = glm::normalize(glm::cross(front, glm::vec3(0,1,0)));
        up    = glm::normalize(glm::cross(right, front));
    }

    glm::mat4 view() { return glm::lookAt(pos, pos + front, up); }

    void moveForward (float dt) { pos += front * speed * dt; }
    void moveBackward(float dt) { pos -= front * speed * dt; }
    void moveLeft    (float dt) { pos -= right * speed * dt; }
    void moveRight   (float dt) { pos += right * speed * dt; }

    void mouse(float dx, float dy)
    {
        yaw   += dx * sensitivity;
        pitch -= dy * sensitivity;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        updateVectors();
    }
};

// ─── Objeto da cena ───────────────────────────────────────────────────────────
struct SceneObject {
    std::string name;
    GLuint      vao;
    int         nVerts;
    GLuint      texID;
    MaterialInfo mat;

    glm::vec3 position;
    glm::vec3 initialPosition;
    glm::vec3 rotationDeg;
    glm::vec3 initialRotationDeg;
    float     scale;
    float     initialScale;
    int       rotateAxis;
    float     rotateStartTime;
    bool      flipNormals;
};

// ─── Bezier cubica ────────────────────────────────────────────────────────────
struct Bezier {
    glm::vec3 p0, p1, p2, p3;
    float t;
    bool  active;
    bool  curveSet;

    glm::vec3 eval()
    {
        float u = 1.0f - t;
        return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
    }
};

// ─── Estado global ────────────────────────────────────────────────────────────
Camera              camera;
std::vector<SceneObject> objects;
int                 selectedObj  = 0;
bool                firstMouse   = true;
double              lastX = SCR_W/2.0, lastY = SCR_H/2.0;
float               deltaTime = 0.0f, lastFrame = 0.0f;
bool                keys[1024]   = {};
Bezier              bezier;
int                 renderMode   = 0;   // 0=normal 1=sem textura 2=so ambient
glm::vec3           lightPos(5.0f, 10.0f, 5.0f);
glm::vec3           initialLightPos(5.0f, 10.0f, 5.0f);
// 3 luzes derivadas da lightPos
glm::vec3 keyLightColor (1.0f,  1.0f,  0.95f);
glm::vec3 fillLightColor(0.4f,  0.4f,  0.5f);
glm::vec3 backLightColor(0.6f,  0.5f,  0.8f);

// ─── Callbacks ────────────────────────────────────────────────────────────────
void key_callback(GLFWwindow* w, int key, int /*sc*/, int action, int /*mod*/)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GLFW_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)   keys[key] = true;
        if (action == GLFW_RELEASE) keys[key] = false;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        selectedObj = (selectedObj + 1) % (int)objects.size();
        std::cout << "Objeto selecionado: " << objects[selectedObj].name << std::endl;
    }

    auto stopRotation = [&](SceneObject& o) {
        if (o.rotateAxis == 0) return;
        float elapsed = (float)glfwGetTime() - o.rotateStartTime;
        if      (o.rotateAxis == 1) o.rotationDeg.x += glm::degrees(elapsed);
        else if (o.rotateAxis == 2) o.rotationDeg.y += glm::degrees(elapsed);
        else if (o.rotateAxis == 3) o.rotationDeg.z += glm::degrees(elapsed);
        o.rotateAxis = 0;
    };
    auto startRotation = [&](SceneObject& o, int axis) {
        stopRotation(o);
        o.rotateAxis      = axis;
        o.rotateStartTime = (float)glfwGetTime();
    };

    if (key == GLFW_KEY_X && action == GLFW_PRESS && !objects.empty())
    {
        SceneObject& sel = objects[selectedObj];
        if (sel.rotateAxis == 1) stopRotation(sel);
        else startRotation(sel, 1);
        std::cout << "Rotacao: X" << std::endl;
    }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS && !objects.empty())
    {
        SceneObject& sel = objects[selectedObj];
        if (sel.rotateAxis == 2) stopRotation(sel);
        else startRotation(sel, 2);
        std::cout << "Rotacao: Y" << std::endl;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS && !objects.empty())
    {
        SceneObject& sel = objects[selectedObj];
        if (sel.rotateAxis == 3) stopRotation(sel);
        else startRotation(sel, 3);
        std::cout << "Rotacao: Z" << std::endl;
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS && !objects.empty())
    {
        stopRotation(objects[selectedObj]);
        std::cout << "Rotacao parada." << std::endl;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        renderMode = (renderMode + 1) % 3;
        const char* names[] = {"Normal (textura + material)", "Sem textura (so material)", "So ambient (ka)"};
        std::cout << "Modo de material: " << names[renderMode] << std::endl;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS && !objects.empty())
    {
        objects[0].position = objects[0].initialPosition;
        bezier.active = false;
        bezier.t      = 0.0f;
        bezier.curveSet = false;
        std::cout << "Bola resetada para posicao inicial." << std::endl;
    }

    if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
        for (auto& o : objects)
        {
            o.position    = o.initialPosition;
            o.rotationDeg = o.initialRotationDeg;
            o.scale       = o.initialScale;
            o.rotateAxis  = 0;
            o.rotateStartTime = 0.0f;
        }
        lightPos      = initialLightPos;
        bezier.active = false;
        bezier.t      = 0.0f;
        bezier.curveSet = false;
        selectedObj   = 0;
        std::cout << "Cena resetada." << std::endl;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        bezier.active = !bezier.active;
        if (bezier.active)
        {
            if (!bezier.curveSet)
            {
                bezier.t = 0.0f;
                bezier.p0 = objects[0].position;
                glm::vec3 goal = objects[1].position;
                float cx = goal.x - 5.0f; // compensa offset geometrico do OBJ da goleira
                bezier.p3 = glm::vec3(cx, goal.y + 1.2f, goal.z - 0.5f);
                bezier.p1 = glm::vec3(cx, bezier.p0.y + 4.0f, bezier.p0.z - 3.0f);
                bezier.p2 = glm::vec3(cx, bezier.p3.y + 3.0f, bezier.p3.z + 2.0f);
                bezier.curveSet = true;
            }
            std::cout << "Trajetoria iniciada/retomada!" << std::endl;
        }
        else
        {
            std::cout << "Trajetoria pausada." << std::endl;
        }
    }
}

void mouse_callback(GLFWwindow* /*w*/, double xpos, double ypos)
{
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float dx = (float)(xpos - lastX);
    float dy = (float)(ypos - lastY);
    lastX = xpos; lastY = ypos;
    camera.mouse(dx, dy);
}

// ─── Shaders ──────────────────────────────────────────────────────────────────
const GLchar* vertSrc = "#version 450\n"
    "layout(location=0) in vec3 position;\n"
    "layout(location=1) in vec3 color;\n"
    "layout(location=2) in vec2 texCoord;\n"
    "layout(location=3) in vec3 normal;\n"
    "uniform mat4 model, view, projection;\n"
    "out vec3 fragPos; out vec3 fragNormal; out vec2 fragUV;\n"
    "void main(){\n"
    "  vec4 wp = model * vec4(position,1.0);\n"
    "  fragPos = wp.xyz;\n"
    "  gl_Position = projection * view * wp;\n"
    "  mat3 NM = transpose(inverse(mat3(model)));\n"
    "  fragNormal = normalize(NM * normal);\n"
    "  fragUV = texCoord;\n"
    "}\n";

const GLchar* fragSrc = "#version 450\n"
    "in vec3 fragPos; in vec3 fragNormal; in vec2 fragUV;\n"
    "uniform sampler2D texture1;\n"
    "uniform vec3 ka, kd, ks;\n"
    "uniform float ns;\n"
    "uniform vec3 keyLightPos, keyLightColor;\n"
    "uniform vec3 fillLightPos, fillLightColor;\n"
    "uniform vec3 backLightPos, backLightColor;\n"
    "uniform vec3 viewPos;\n"
    "uniform bool flipNormals;\n"
    "uniform int renderMode;\n"
    "out vec4 outColor;\n"
    "vec3 calcLight(vec3 lPos, vec3 lColor, vec3 N, vec3 V, vec3 tex){\n"
    "  vec3 L = normalize(lPos - fragPos);\n"
    "  vec3 R = reflect(-L, N);\n"
    "  vec3 diff = kd * tex * lColor * max(dot(N,L), 0.0);\n"
    "  vec3 spec = ks * lColor * pow(max(dot(R,V),0.0), ns);\n"
    "  return diff + spec;\n"
    "}\n"
    "void main(){\n"
    "  vec3 N = normalize(fragNormal);\n"
    "  if (flipNormals) N = -N;\n"
    "  vec3 V = normalize(viewPos - fragPos);\n"
    "  vec3 tex = (renderMode == 0) ? texture(texture1, fragUV).rgb : vec3(1.0);\n"
    "  vec3 ambient = ka * 0.1;\n"
    "  if (renderMode == 2) { outColor = vec4(ka, 1.0); return; }\n"
    "  vec3 lighting = calcLight(keyLightPos,  keyLightColor,  N, V, tex)\n"
    "               + calcLight(fillLightPos, fillLightColor, N, V, tex)\n"
    "               + calcLight(backLightPos, backLightColor, N, V, tex);\n"
    "  outColor = vec4(ambient + lighting, 1.0);\n"
    "}\n";

GLuint buildShader()
{
    auto compile = [](GLenum type, const GLchar* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, NULL);
        glCompileShader(s);
        GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[512]; glGetShaderInfoLog(s,512,NULL,log); std::cerr << log << std::endl; }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,   vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    return prog;
}

// ─── Chao ─────────────────────────────────────────────────────────────────────
GLuint buildGroundVAO(float size, float tiling)
{
    float h = size * 0.5f;
    float t = tiling;
    float verts[] = {
        // pos(3)            color(3)          uv(2)    normal(3)
        -h, 0,  h,   1,1,1,  0,  0,    0,1,0,
         h, 0,  h,   1,1,1,  t,  0,    0,1,0,
         h, 0, -h,   1,1,1,  t,  t,    0,1,0,
        -h, 0,  h,   1,1,1,  0,  0,    0,1,0,
         h, 0, -h,   1,1,1,  t,  t,    0,1,0,
        -h, 0, -h,   1,1,1,  0,  t,    0,1,0,
    };
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    GLsizei stride = 11 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8*sizeof(float)));
    glEnableVertexAttribArray(3);
    glBindVertexArray(0);
    return VAO;
}

// ─── Parser JSON minimalista ──────────────────────────────────────────────────
static std::string jsonStr(const std::string& json, const std::string& key)
{
    std::string k = "\"" + key + "\"";
    size_t p = json.find(k);
    if (p == std::string::npos) return "";
    p += k.size();
    // pula até ':'
    p = json.find(':', p);
    if (p == std::string::npos) return "";
    ++p;
    // pula espaços/tabs/newlines
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\n' || json[p] == '\r')) ++p;
    if (p >= json.size() || json[p] != '"') return "";
    ++p; // pula o " de abertura
    size_t r = json.find('"', p);
    if (r == std::string::npos) return "";
    return json.substr(p, r - p);
}

static float jsonFloat(const std::string& json, const std::string& key, float def = 0.0f)
{
    std::string k = "\"" + key + "\"";
    size_t p = json.find(k);
    if (p == std::string::npos) return def;
    p = json.find(':', p);
    if (p == std::string::npos) return def;
    ++p;
    while (p < json.size() && (json[p]==' '||json[p]=='\t'||json[p]=='\n'||json[p]=='\r')) ++p;
    return std::stof(json.substr(p));
}

static glm::vec3 jsonVec3(const std::string& json, const std::string& key)
{
    std::string k = "\"" + key + "\"";
    size_t p = json.find(k);
    if (p == std::string::npos) return glm::vec3(0);
    p = json.find('[', p);
    if (p == std::string::npos) return glm::vec3(0);
    size_t q = json.find(']', p);
    std::string arr = json.substr(p+1, q-p-1);
    std::istringstream ss(arr);
    float x=0,y=0,z=0; char comma;
    ss >> x >> comma >> y >> comma >> z;
    return glm::vec3(x,y,z);
}

// ─── Carrega um objeto a partir de um bloco JSON de objeto ────────────────────
SceneObject loadObjectFromJSON(const std::string& block, const std::string& exeDir)
{
    SceneObject obj;
    obj.name       = jsonStr(block, "name");
    obj.position        = jsonVec3(block, "position");
    obj.initialPosition = obj.position;
    obj.rotationDeg     = jsonVec3(block, "rotation");
    obj.scale           = jsonFloat(block, "scale", 1.0f);
    obj.initialRotationDeg = obj.rotationDeg;
    obj.initialScale       = obj.scale;
    obj.rotateAxis      = 0;
    obj.rotateStartTime = 0.0f;
    obj.flipNormals     = (obj.name == "bola");

    std::string objPath = exeDir + "/" + jsonStr(block, "obj");

    obj.vao = loadSimpleOBJ(objPath, obj.nVerts);

    if (obj.name == "bola")
    {
        obj.mat.ka = glm::vec3(0.1f);
        obj.mat.kd = glm::vec3(0.8f, 0.8f, 0.85f);
        obj.mat.ks = glm::vec3(1.0f);
        obj.mat.ns = 64.0f;
        std::string texPath = jsonStr(block, "texture");
        obj.texID  = texPath.empty() ? createWhiteTexture() : loadTexture(exeDir + "/" + texPath);
    }
    else
    {
        std::string mtlPath = jsonStr(block, "mtl");
        std::string texPath = jsonStr(block, "texture");

        if (!mtlPath.empty())
            obj.mat = loadMaterialInfo(exeDir + "/" + mtlPath);
        else
        {
            obj.mat.ka = glm::vec3(0.2f);
            obj.mat.kd = glm::vec3(0.8f);
            obj.mat.ks = glm::vec3(0.5f);
            obj.mat.ns = 32.0f;
        }

        if (!texPath.empty())
            obj.texID = loadTexture(exeDir + "/" + texPath);
        else if (!obj.mat.textureName.empty())
        {
            std::string dir = objPath.substr(0, objPath.find_last_of('/'));
            obj.texID = loadTexture(dir + "/" + obj.mat.textureName);
        }
        else
            obj.texID = createWhiteTexture();
    }

    return obj;
}

// ─── Extrai blocos de array JSON para "objects" ───────────────────────────────
std::vector<std::string> extractObjectBlocks(const std::string& json)
{
    std::vector<std::string> blocks;
    size_t arrStart = json.find("\"objects\"");
    if (arrStart == std::string::npos) return blocks;
    arrStart = json.find('[', arrStart);
    if (arrStart == std::string::npos) return blocks;

    // Encontra o ] que fecha o array "objects" rastreando profundidade de colchetes
    int bracketDepth = 0;
    size_t arrEnd = arrStart;
    for (size_t i = arrStart; i < json.size(); ++i)
    {
        if      (json[i] == '[') ++bracketDepth;
        else if (json[i] == ']') { --bracketDepth; if (bracketDepth == 0) { arrEnd = i; break; } }
    }

    int depth = 0;
    size_t blockStart = std::string::npos;
    for (size_t i = arrStart; i <= arrEnd; ++i)
    {
        if (json[i] == '{')
        {
            if (depth == 0) blockStart = i;
            ++depth;
        }
        else if (json[i] == '}')
        {
            --depth;
            if (depth == 0 && blockStart != std::string::npos)
            {
                blocks.push_back(json.substr(blockStart, i - blockStart + 1));
                blockStart = std::string::npos;
            }
        }
    }
    return blocks;
}

// ─── main ─────────────────────────────────────────────────────────────────────
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Projeto Final - Cena de Futebol", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    { std::cerr << "GLAD falhou" << std::endl; return -1; }

    int fw, fh;
    glfwGetFramebufferSize(window, &fw, &fh);
    glViewport(0, 0, fw, fh);
    glEnable(GL_DEPTH_TEST);

    GLuint shader = buildShader();

    std::string exeDir = getExeDir();
    std::string scenePath = exeDir + "/../Grau-B-Projeto-Final/scene.json";

    // Carrega o JSON inteiro
    std::ifstream sceneFile(scenePath);
    if (!sceneFile.is_open())
    { std::cerr << "scene.json nao encontrado em: " << scenePath << std::endl; return -1; }
    std::string json((std::istreambuf_iterator<char>(sceneFile)), std::istreambuf_iterator<char>());
    sceneFile.close();

    // Extrai blocos de cada secao por nome
    auto extractBlock = [&](const std::string& section) -> std::string {
        std::string key = "\"" + section + "\"";
        size_t p = json.find(key);
        if (p == std::string::npos) return "";
        size_t b = json.find('{', p);
        if (b == std::string::npos) return "";
        int depth = 0; size_t e = b;
        for (; e < json.size(); ++e) {
            if (json[e] == '{') ++depth;
            else if (json[e] == '}') { --depth; if (depth == 0) break; }
        }
        return json.substr(b, e - b + 1);
    };

    std::string camBlock    = extractBlock("camera");
    std::string lightBlock  = extractBlock("light");
    std::string groundBlock = extractBlock("ground");

    // Camera
    camera.pos         = jsonVec3  (camBlock, "position");
    camera.yaw         = jsonFloat (camBlock, "yaw",         -90.0f);
    camera.pitch       = jsonFloat (camBlock, "pitch",         0.0f);
    camera.fov         = jsonFloat (camBlock, "fov",          45.0f);
    camera.speed       = jsonFloat (camBlock, "speed",         5.0f);
    camera.sensitivity = jsonFloat (camBlock, "sensitivity",   0.02f);
    camera.updateVectors();

    float nearPlane = jsonFloat(camBlock, "near",   0.1f);
    float farPlane  = jsonFloat(camBlock, "far",  200.0f);

    // Luz (lightPos e global, inicializa com valor do JSON)
    lightPos        = jsonVec3(lightBlock, "position");
    initialLightPos = lightPos;
    glm::vec3 lightColor = jsonVec3(lightBlock, "color");

    // Objetos
    std::vector<std::string> objBlocks = extractObjectBlocks(json);
    for (auto& block : objBlocks)
        objects.push_back(loadObjectFromJSON(block, exeDir));

    if (objects.empty())
    { std::cerr << "Nenhum objeto carregado!" << std::endl; return -1; }

    // Chao
    float groundSize   = jsonFloat(groundBlock, "size",   30.0f);
    float groundTiling = jsonFloat(groundBlock, "tiling",  6.0f);
    GLuint groundVAO   = buildGroundVAO(groundSize, groundTiling);
    std::string groundTexPath = groundBlock.empty() ? "" : exeDir + "/" + jsonStr(groundBlock, "texture");
    GLuint groundTex = groundTexPath.empty() ? createWhiteTexture() : loadTexture(groundTexPath);

    MaterialInfo groundMat;
    groundMat.ka = glm::vec3(0.3f);
    groundMat.kd = glm::vec3(0.7f);
    groundMat.ks = glm::vec3(0.05f);
    groundMat.ns = 4.0f;

    // Uniforms
    glUseProgram(shader);
    GLint uModel    = glGetUniformLocation(shader, "model");
    GLint uView     = glGetUniformLocation(shader, "view");
    GLint uProj     = glGetUniformLocation(shader, "projection");
    GLint uKa       = glGetUniformLocation(shader, "ka");
    GLint uKd       = glGetUniformLocation(shader, "kd");
    GLint uKs       = glGetUniformLocation(shader, "ks");
    GLint uNs       = glGetUniformLocation(shader, "ns");
    GLint uKeyLightPos  = glGetUniformLocation(shader, "keyLightPos");
    GLint uKeyLightCol  = glGetUniformLocation(shader, "keyLightColor");
    GLint uFillLightPos = glGetUniformLocation(shader, "fillLightPos");
    GLint uFillLightCol = glGetUniformLocation(shader, "fillLightColor");
    GLint uBackLightPos = glGetUniformLocation(shader, "backLightPos");
    GLint uBackLightCol = glGetUniformLocation(shader, "backLightColor");
    GLint uViewPos      = glGetUniformLocation(shader, "viewPos");
    GLint uFlipNormals  = glGetUniformLocation(shader, "flipNormals");
    GLint uRenderMode   = glGetUniformLocation(shader, "renderMode");
    GLint uTex          = glGetUniformLocation(shader, "texture1");

    glUniform3fv(uKeyLightCol,  1, glm::value_ptr(keyLightColor));
    glUniform3fv(uFillLightCol, 1, glm::value_ptr(fillLightColor));
    glUniform3fv(uBackLightCol, 1, glm::value_ptr(backLightColor));

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.fov),
        (float)SCR_W / (float)SCR_H,
        nearPlane, farPlane);
    glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(projection));

    bezier.active = false;
    bezier.t      = 0.0f;
    bezier.curveSet = false;

    std::cout << "\n=== Projeto Final - Cena de Futebol ===" << std::endl;
    std::cout << "WASD        : mover camera" << std::endl;
    std::cout << "Mouse       : orientar camera" << std::endl;
    std::cout << "TAB         : selecionar objeto" << std::endl;
    std::cout << "Setas       : mover objeto (X/Z)" << std::endl;
    std::cout << "I / K       : mover objeto (Y)" << std::endl;
    std::cout << "R           : ligar/desligar rotacao" << std::endl;
    std::cout << "[ / ]       : diminuir / aumentar escala" << std::endl;
    std::cout << "ESPACO      : iniciar/parar chute da bola" << std::endl;
    std::cout << "M           : alternar modo (textura / so material / so ambient)" << std::endl;
    std::cout << "J/L/U/O/P/N : mover luz (X/Y/Z)" << std::endl;
    std::cout << "ESC         : sair" << std::endl;

    // ─── Loop principal ───────────────────────────────────────────────────────
    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        deltaTime = now - lastFrame;
        lastFrame = now;

        glfwPollEvents();

        // Movimento da camera
        if (keys[GLFW_KEY_W]) camera.moveForward (deltaTime);
        if (keys[GLFW_KEY_S]) camera.moveBackward(deltaTime);
        if (keys[GLFW_KEY_A]) camera.moveLeft    (deltaTime);
        if (keys[GLFW_KEY_D]) camera.moveRight   (deltaTime);

        // Movimento do objeto selecionado
        const float objStep = 3.0f * deltaTime;
        if (!objects.empty())
        {
            SceneObject& sel = objects[selectedObj];
            if (keys[GLFW_KEY_RIGHT]) sel.position.x += objStep;
            if (keys[GLFW_KEY_LEFT])  sel.position.x -= objStep;
            if (keys[GLFW_KEY_UP])    sel.position.z -= objStep;
            if (keys[GLFW_KEY_DOWN])  sel.position.z += objStep;
            if (keys[GLFW_KEY_I])     sel.position.y += objStep;
            if (keys[GLFW_KEY_K])     sel.position.y -= objStep;
            if (keys[GLFW_KEY_RIGHT_BRACKET]) sel.scale += 0.5f * deltaTime;
            if (keys[GLFW_KEY_LEFT_BRACKET])  sel.scale = std::max(0.001f, sel.scale - 0.5f * deltaTime);
        }

        // Controle da luz (J/L = X, U/O = Y, P/N = Z)
        const float lightStep = 8.0f * deltaTime;
        if (keys[GLFW_KEY_J]) lightPos.x -= lightStep;
        if (keys[GLFW_KEY_L]) lightPos.x += lightStep;
        if (keys[GLFW_KEY_U]) lightPos.y += lightStep;
        if (keys[GLFW_KEY_O]) lightPos.y -= lightStep;
        if (keys[GLFW_KEY_P]) lightPos.z -= lightStep;
        if (keys[GLFW_KEY_N]) lightPos.z += lightStep;

        // Trajetoria Bezier da bola (objeto 0)
        if (bezier.active && !objects.empty())
        {
            bezier.t += deltaTime * 0.4f;
            if (bezier.t >= 1.0f)
            {
                bezier.t = 1.0f;
                bezier.active = false;
                std::cout << "GOL! Trajetoria concluida." << std::endl;
            }
            objects[0].position = bezier.eval();
        }

        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);   // ceu azul claro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);

        glm::mat4 view = camera.view();
        glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(uViewPos, 1, glm::value_ptr(camera.pos));
        glUniform1i(uRenderMode, renderMode);
        // 3 luzes: key (frontal), fill (oposto), back (atras)
        glm::vec3 keyPos  = lightPos;
        glm::vec3 fillPos = glm::vec3(-lightPos.x, lightPos.y * 0.6f,  lightPos.z);
        glm::vec3 backPos = glm::vec3( 0.0f,        lightPos.y * 0.8f, -lightPos.z * 1.5f);
        glUniform3fv(uKeyLightPos,  1, glm::value_ptr(keyPos));
        glUniform3fv(uFillLightPos, 1, glm::value_ptr(fillPos));
        glUniform3fv(uBackLightPos, 1, glm::value_ptr(backPos));

        // Desenha objetos da cena
        for (int i = 0; i < (int)objects.size(); ++i)
        {
            SceneObject& o = objects[i];

            glm::mat4 model(1.0f);
            model = glm::translate(model, o.position);
            float elapsed = (o.rotateAxis != 0) ? (now - o.rotateStartTime) : 0.0f;
            glm::vec3 animDeg(0.0f);
            if      (o.rotateAxis == 1) animDeg.x = glm::degrees(elapsed);
            else if (o.rotateAxis == 2) animDeg.y = glm::degrees(elapsed);
            else if (o.rotateAxis == 3) animDeg.z = glm::degrees(elapsed);
            model = glm::rotate(model, glm::radians(o.rotationDeg.x + animDeg.x), glm::vec3(1,0,0));
            model = glm::rotate(model, glm::radians(o.rotationDeg.y + animDeg.y), glm::vec3(0,1,0));
            model = glm::rotate(model, glm::radians(o.rotationDeg.z + animDeg.z), glm::vec3(0,0,1));
            model = glm::scale (model, glm::vec3(o.scale));

            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(uKa, 1, glm::value_ptr(o.mat.ka));
            glUniform3fv(uKd, 1, glm::value_ptr(o.mat.kd));
            glUniform3fv(uKs, 1, glm::value_ptr(o.mat.ks));
            glUniform1f (uNs, o.mat.ns);
            glUniform1i (uFlipNormals, o.flipNormals ? 1 : 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, o.texID);
            glUniform1i(uTex, 0);

            glBindVertexArray(o.vao);
            glDrawArrays(GL_TRIANGLES, 0, o.nVerts);
            glBindVertexArray(0);
        }

        // Desenha chao
        {
            glm::mat4 model(1.0f);
            glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(uKa, 1, glm::value_ptr(groundMat.ka));
            glUniform3fv(uKd, 1, glm::value_ptr(groundMat.kd));
            glUniform3fv(uKs, 1, glm::value_ptr(groundMat.ks));
            glUniform1f (uNs, groundMat.ns);
            glUniform1i (uFlipNormals, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, groundTex);
            glUniform1i(uTex, 0);
            glBindVertexArray(groundVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
