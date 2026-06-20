#ifndef LOADSIMPLEOBJ_H
#define LOADSIMPLEOBJ_H

#include <string>

#include <glad/glad.h>

// Carrega um arquivo .OBJ simples e retorna o VAO gerado.
// O buffer de vrtices conter: posio (3), cor (3) e coordenadas de textura (2).
int loadSimpleOBJ(std::string filePATH, int &nVertices);

// L o arquivo .MTL associado e retorna o nome do arquivo de textura
// indicado pela primeira instruo map_Kd encontrada.
// Retorna string vazia caso no haja textura.
std::string loadMTLTextureName(std::string filePATH);

// Carrega uma imagem de textura usando stb_image e retorna o ID do OpenGL.
GLuint loadTexture(std::string filePATH);

#endif
