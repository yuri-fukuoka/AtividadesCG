# Computação Gráfica — OpenGL

Repositório com os projetos desenvolvidos na disciplina de Computação Gráfica, utilizando OpenGL moderno (versão 4.5), GLFW, GLAD e GLM.

## Estrutura

| Pasta | Descrição |
|---|---|
| `Grau-A-M1-Hello3d` | Triângulo 3D básico |
| `Grau-A-M2-Cubo` | Cubo com transformações |
| `Grau-A-M3-Texturas` | Cubo com textura |
| `Grau-A-M4-Iluminacao` | Iluminação Phong |
| `Grau-A-M5-Camera` | Câmera em primeira pessoa |
| `Grau-A-M6-Trajetorias` | Trajetórias e animação |
| `Grau-B-Vivencial-1` | Visualizador e Manipulador de Objetos 3D em OpenGL |
| `Grau-B-Vivencial-2` | Iluminação de 3 pontos com bola |
| `Grau-B-Projeto-Final` | Cena de futebol completa |
| `Assets/` | Modelos OBJ, texturas e materiais compartilhados |

## Compilação

```bash
mkdir -p build && cd build
cmake ..
make
```

Os executáveis são gerados na pasta `build/`.

## Dependências

- OpenGL 4.5+
- GLFW3
- GLAD
- GLM
- stb_image (incluso em `include/`)
