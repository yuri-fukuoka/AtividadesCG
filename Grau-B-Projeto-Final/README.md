# Projeto Final — Cena de Futebol

Cena 3D interativa com bola e goleira, iluminação Phong de 3 pontos, câmera em primeira pessoa, seleção e transformação de objetos, e animação da bola por curva de Bézier cúbica.

## Compilação

```bash
cd build
make ProjetoFinal
./ProjetoFinal
```

A cena é configurada via `Grau-B-Projeto-Final/scene.json` (posição da câmera, luz, objetos e chão).

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

## Assets Utilizados

| Asset | Uso |
|---|---|
| `Assets/Pallone/Ball OBJ.obj` | Modelo da bola |
| `Assets/Football Goal/football goal.obj` | Modelo da goleira |
| `Assets/Ground003_1K-JPG/Ground003_1K-JPG_Color.jpg` | Textura do chão |
