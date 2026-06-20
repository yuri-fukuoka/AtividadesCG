cmake_minimum_required(VERSION 3.10)
project(ComputacaoGrafica)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Grau A - M1
add_executable(Hello3D
    Grau-A-M1-Hello3d/src/Hello3D.cpp
    Grau-A-M1-Hello3d/src/glad.c
)
target_link_libraries(Hello3D glfw GL dl pthread)
target_include_directories(Hello3D PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M1-Hello3d/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau A - M2
add_executable(M2
    Grau-A-M2-Cubo/src/M2.cpp
    Grau-A-M2-Cubo/src/glad.c
)
target_link_libraries(M2 glfw GL dl pthread)
target_include_directories(M2 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M2-Cubo/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau A - M3
add_executable(M3
    Grau-A-M3-Texturas/src/M3.cpp
    Grau-A-M3-Texturas/src/LoadSimpleOBJ.cpp
    Grau-A-M2-Cubo/src/glad.c
)
target_link_libraries(M3 glfw GL dl pthread)
target_include_directories(M3 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M3-Texturas/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau A - M4
add_executable(M4
    Grau-A-M4-Iluminacao/src/M4.cpp
    Grau-A-M4-Iluminacao/src/LoadSimpleOBJ.cpp
    Grau-A-M4-Iluminacao/src/glad.c
)
target_link_libraries(M4 glfw GL dl pthread)
target_include_directories(M4 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M4-Iluminacao/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau A - M5
add_executable(M5
    Grau-A-M5-Camera/src/M5.cpp
    Grau-A-M5-Camera/src/Camera.cpp
    Grau-A-M5-Camera/src/LoadSimpleOBJ.cpp
    Grau-A-M5-Camera/src/glad.c
)
target_link_libraries(M5 glfw GL dl pthread)
target_include_directories(M5 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M5-Camera/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau B - Vivencial 2 - Iluminacao de 3 Pontos
add_executable(Vivencial2
    Grau-B-Vivencial-2/src/Vivencial2.cpp
    Grau-B-Vivencial-2/src/LoadSimpleOBJ.cpp
    Grau-B-Vivencial-2/src/glad.c
)
target_link_libraries(Vivencial2 glfw GL dl pthread)
target_include_directories(Vivencial2 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-B-Vivencial-2/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau B - Projeto Final
add_executable(ProjetoFinal
    Grau-B-Projeto-Final/src/ProjetoFinal.cpp
    Grau-B-Projeto-Final/src/LoadSimpleOBJ.cpp
    Grau-B-Projeto-Final/src/glad.c
)
target_link_libraries(ProjetoFinal glfw GL dl pthread)
target_include_directories(ProjetoFinal PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-B-Projeto-Final/src
    ${CMAKE_SOURCE_DIR}/include
)

# Grau A - M6
add_executable(M6
    Grau-A-M6-Trajetorias/src/M6.cpp
    Grau-A-M6-Trajetorias/src/LoadSimpleOBJ.cpp
    Grau-A-M6-Trajetorias/src/glad.c
)
target_link_libraries(M6 glfw GL dl pthread)
target_include_directories(M6 PRIVATE
    ${CMAKE_SOURCE_DIR}/Grau-A-M6-Trajetorias/src
    ${CMAKE_SOURCE_DIR}/include
)