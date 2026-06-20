#include "LoadSimpleOBJ.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int loadSimpleOBJ(std::string filePATH, int &nVertices)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat>   vBuffer;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
    {
        std::cerr << "Erro ao abrir OBJ: " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 vn;
            ssline >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            struct Idx { int vi, ti, ni; };
            std::vector<Idx> fi;

            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string idx;

                if (std::getline(ss, idx, '/')) vi = !idx.empty() ? std::stoi(idx) - 1 : 0;
                if (std::getline(ss, idx, '/')) ti = !idx.empty() ? std::stoi(idx) - 1 : 0;
                if (std::getline(ss, idx))       ni = !idx.empty() ? std::stoi(idx) - 1 : 0;

                fi.push_back({vi, ti, ni});
            }

            for (size_t i = 1; i + 1 < fi.size(); ++i)
            {
                size_t idxs[3] = {0, i, i + 1};

                // Calcula normal da face por cross product se o OBJ nao tem normais
                glm::vec3 faceNormal(0,1,0);
                if (normals.empty())
                {
                    glm::vec3 p0 = vertices[fi[idxs[0]].vi];
                    glm::vec3 p1 = vertices[fi[idxs[1]].vi];
                    glm::vec3 p2 = vertices[fi[idxs[2]].vi];
                    glm::vec3 e1 = p1 - p0;
                    glm::vec3 e2 = p2 - p0;
                    glm::vec3 c  = glm::cross(e1, e2);
                    if (glm::length(c) > 0.0f) faceNormal = glm::normalize(c);
                }

                for (size_t k = 0; k < 3; ++k)
                {
                    int vi = fi[idxs[k]].vi;
                    int ti = fi[idxs[k]].ti;
                    int ni = fi[idxs[k]].ni;

                    glm::vec3 pos = (vi >= 0 && vi < (int)vertices.size())  ? vertices[vi]  : glm::vec3(0);
                    glm::vec3 n   = normals.empty() ? faceNormal
                                  : ((ni >= 0 && ni < (int)normals.size()) ? normals[ni] : glm::vec3(0,1,0));
                    glm::vec2 uv;
                    if (!texCoords.empty() && ti >= 0 && ti < (int)texCoords.size())
                        uv = texCoords[ti];
                    else {
                        // UV esferica a partir da normal (sphere mapping)
                        glm::vec3 nn = glm::length(n) > 0.0f ? glm::normalize(n) : glm::normalize(pos);
                        uv.s = 0.5f + atan2f(nn.z, nn.x) / (2.0f * 3.14159265f);
                        uv.t = 0.5f - asinf(glm::clamp(nn.y, -1.0f, 1.0f)) / 3.14159265f;
                    }

                    // pos(3) + color(3) + uv(2) + normal(3) = 11
                    vBuffer.push_back(pos.x); vBuffer.push_back(pos.y); vBuffer.push_back(pos.z);
                    vBuffer.push_back(1.0f);  vBuffer.push_back(1.0f);  vBuffer.push_back(1.0f);
                    vBuffer.push_back(uv.s);  vBuffer.push_back(uv.t);
                    vBuffer.push_back(n.x);   vBuffer.push_back(n.y);   vBuffer.push_back(n.z);
                }
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLsizei stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = (int)(vBuffer.size() / 11);
    std::cout << "OBJ carregado: " << filePATH << " (" << nVertices << " vertices)" << std::endl;
    return (int)VAO;
}

MaterialInfo loadMaterialInfo(std::string filePATH)
{
    MaterialInfo mat;
    mat.ka = glm::vec3(0.2f);
    mat.kd = glm::vec3(0.8f);
    mat.ks = glm::vec3(0.5f);
    mat.ns = 32.0f;
    mat.textureName = "";

    std::ifstream f(filePATH.c_str());
    if (!f.is_open()) return mat;

    std::string line;
    while (std::getline(f, line))
    {
        std::istringstream ss(line);
        std::string word;
        ss >> word;

        if      (word == "Ka") ss >> mat.ka.r >> mat.ka.g >> mat.ka.b;
        else if (word == "Kd") ss >> mat.kd.r >> mat.kd.g >> mat.kd.b;
        else if (word == "Ks") ss >> mat.ks.r >> mat.ks.g >> mat.ks.b;
        else if (word == "Ns") ss >> mat.ns;
        else if (word == "map_Kd") ss >> mat.textureName;
    }
    return mat;
}

GLuint loadTexture(std::string filePATH)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filePATH.c_str(), &w, &h, &ch, 0);
    if (data)
    {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Textura carregada: " << filePATH << " (" << w << "x" << h << ")" << std::endl;
    }
    else
    {
        std::cerr << "Erro ao carregar textura: " << filePATH << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

GLuint createWhiteTexture()
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
