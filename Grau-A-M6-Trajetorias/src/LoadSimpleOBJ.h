#ifndef LOADSIMPLEOBJ_H
#define LOADSIMPLEOBJ_H

#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

// Estrutura com os coeficientes de material lidos do .MTL
struct MaterialInfo
{
    glm::vec3 ka;          // Ambiente
    glm::vec3 kd;          // Difuso
    glm::vec3 ks;          // Especular
    float ns;              // Exponente de brilho
    std::string textureName;
};

// Carrega um arquivo .OBJ simples e retorna o VAO gerado.
// O buffer de vertices contem: posicao(3), cor(3), coordenadas de textura(2) e normal(3).
int loadSimpleOBJ(std::string filePATH, int &nVertices);

// Le o arquivo .MTL associado e retorna os coeficientes Ka, Kd, Ks e Ns,
// alem do nome da textura indicada por map_Kd.
MaterialInfo loadMaterialInfo(std::string filePATH);

// Carrega uma imagem de textura usando stb_image e retorna o ID do OpenGL.
GLuint loadTexture(std::string filePATH);

#endif
