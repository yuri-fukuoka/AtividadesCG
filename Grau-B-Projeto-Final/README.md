# Projeto Final — Cena de Futebol

Cena 3D interativa com bola e goleira, iluminação Phong de 3 pontos, câmera em primeira pessoa, seleção e transformação de objetos, e animação da bola por curva de Bézier cúbica.

## Setup

### Compilação

```bash
cd build
cmake ..
make ProjetoFinal
./ProjetoFinal
```

A cena é configurada via `Grau-B-Projeto-Final/scene.json` (posição da câmera, luz, objetos e chão).

### Dependências

- OpenGL 4.5+
- GLFW3
- GLAD
- GLM
- stb_image (já incluso em `include/`)

Todas as dependências devem estar disponíveis no sistema antes da compilação. No Ubuntu/Debian, instale com:

```bash
sudo apt update
sudo apt install libglfw3-dev libglm-dev libgl1-mesa-dev
```

## Controles

### Câmera
| Tecla / Entrada | Ação |
|---|---|
| `W / A / S / D` | Mover câmera (frente/esquerda/trás/direita) |
| `Mouse` | Orientar câmera (yaw/pitch) |

### Seleção e Transformação de Objetos
| Tecla | Ação |
|---|---|
| `TAB` | Alternar objeto selecionado (bola / goleira) |
| `Setas` | Mover objeto selecionado (X / Z) |
| `I / K` | Mover objeto selecionado (Y) |
| `[ / ]` | Diminuir / aumentar escala |
| `X` | Rotacionar no eixo X |
| `Y` | Rotacionar no eixo Y |
| `Z` | Rotacionar no eixo Z |
| `R` | Parar rotação (acumula ângulo atual) |

### Animação
| Tecla | Ação |
|---|---|
| `ESPAÇO` | Iniciar / pausar chute da bola (curva de Bézier) |
| `H` | Resetar posição da bola |
| `F5` | Resetar cena completa (posição, rotação e escala de todos os objetos + luz) |

### Luz
| Tecla | Ação |
|---|---|
| `J / L` | Mover luz no eixo X |
| `U / O` | Mover luz no eixo Y (cima/baixo) |
| `P / ;` | Mover luz no eixo Z |

### Geral
| Tecla | Ação |
|---|---|
| `ESC` | Sair |

## Assets

| Asset | Uso | Fonte |
|---|---|---|
| `Assets/Pallone/Ball OBJ.obj` | Modelo da bola | [free3d.com — Football Ball](https://free3d.com/3d-model/football-ball--64059.html) |
| `Assets/Football Goal/football goal.obj` | Modelo da goleira | [free3d.com — Football Goal](https://free3d.com/3d-model/football-goal-473670.html) |
| `Assets/Ground003_1K-JPG/Ground003_1K-JPG_Color.jpg` | Textura do chão (grama/terra) | [ambientCG — Ground003](https://ambientcg.com/view?id=Ground003) |

### Licenças e Processamento

Os modelos 3D e a textura foram obtidos nos sites listados acima e são utilizados conforme as licenças de uso de cada plataforma. Os arquivos `.obj` e `.jpg` foram utilizados diretamente no projeto, sem processamento prévio em softwares como Blender ou MeshLab.

- **free3d.com**: disponibiliza modelos gratuitos sujeitos às condições de uso do site. Consulte a página de cada asset para detalhes da licença.
- **ambientCG**: texturas geralmente disponíveis sob licença Creative Commons Zero (CC0) / domínio público dedicado.

## Referências

### Documentação Técnica

- [OpenGL 4.5 Reference Pages](https://www.khronos.org/registry/OpenGL-Refpages/gl4/)
- [GLFW Documentation](https://www.glfw.org/documentation.html)
- [GLM — OpenGL Mathematics](https://glm.g-truc.net/)
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) — carregamento de texturas

### Recursos e Tutoriais

- Joey de Vries. *Learn OpenGL — Graphics Programming*. Disponível em: <https://learnopengl.com/>
- Slides e materiais da disciplina de Computação Gráfica.

### Bibliografia

- Hearn, D., Baker, M. P., & Carithers, W. R. *Computer Graphics with OpenGL*. 4th ed. Pearson, 2011.
- Angel, E., & Shreiner, D. *Interactive Computer Graphics: A Top-Down Approach with WebGL*. 7th ed. Pearson, 2015.
