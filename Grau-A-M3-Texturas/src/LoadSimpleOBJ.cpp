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
    std::vector<GLfloat> vBuffer;

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
    {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    int faceIndex = 0;
    while (std::getline(arqEntrada, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "v")
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f")
        {
            glm::vec3 faceColors[6] = {
                glm::vec3(1.0f, 0.0f, 0.0f), // Frente - vermelho
                glm::vec3(0.0f, 1.0f, 0.0f), // Trs - verde
                glm::vec3(1.0f, 1.0f, 0.0f), // Direita - amarelo
                glm::vec3(0.0f, 0.0f, 1.0f), // Esquerda - azul
                glm::vec3(0.0f, 1.0f, 1.0f), // Cima - ciano
                glm::vec3(1.0f, 0.0f, 1.0f)  // Baixo - magenta
            };

            glm::vec3 color = faceColors[faceIndex % 6];
            faceIndex++;

            struct FaceIndex { int vi, ti, ni; };
            std::vector<FaceIndex> faceIndices;

            while (ssline >> word)
            {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index))       ni = !index.empty() ? std::stoi(index) - 1 : 0;

                faceIndices.push_back({vi, ti, ni});
            }

            // Triangulao em fan para polgonos com N vrtices
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i)
            {
                size_t indices[3] = {0, i, i + 1};
                for (size_t k = 0; k < 3; ++k)
                {
                    size_t idx = indices[k];
                    int vi = faceIndices[idx].vi;
                    int ti = faceIndices[idx].ti;

                    vBuffer.push_back(vertices[vi].x);
                    vBuffer.push_back(vertices[vi].y);
                    vBuffer.push_back(vertices[vi].z);
                    vBuffer.push_back(color.r);
                    vBuffer.push_back(color.g);
                    vBuffer.push_back(color.b);
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }
            }
        }
    }

    arqEntrada.close();

    std::cout << "Gerando o buffer de geometria..." << std::endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Atributo posio (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Atributo coordenadas de textura (s, t)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8;

    return VAO;
}

std::string loadMTLTextureName(std::string filePATH)
{
    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open())
    {
        std::cerr << "Erro ao tentar ler o arquivo MTL " << filePATH << std::endl;
        return "";
    }

    std::string line;
    while (std::getline(arqEntrada, line))
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "map_Kd")
        {
            std::string textureName;
            ssline >> textureName;
            return textureName;
        }
    }

    return "";
}

GLuint loadTexture(std::string filePATH)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filePATH.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Textura carregada: " << filePATH << " (" << width << "x" << height << ", " << nrChannels << " canais)" << std::endl;
    }
    else
    {
        std::cerr << "Erro ao carregar textura " << filePATH << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}
