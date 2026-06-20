#ifndef LOADSIMPLEOBJ_H
#define LOADSIMPLEOBJ_H

#include <string>

#include <glad/glad.h>
#include <glm/glm.hpp>

struct MaterialInfo
{
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float ns;
    std::string textureName;
};

int loadSimpleOBJ(std::string filePATH, int &nVertices);
MaterialInfo loadMaterialInfo(std::string filePATH);
GLuint loadTexture(std::string filePATH);

#endif
