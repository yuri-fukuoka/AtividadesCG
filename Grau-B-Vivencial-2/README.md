# Vivencial 2 — Iluminação de 3 Pontos

Cena OpenGL com uma bola de futebol iluminada pela técnica de **iluminação de 3 pontos** (three-point lighting), com modelo de sombreamento Phong.

## Técnica de Iluminação

| Luz | Descrição |
|---|---|
| **Key Light** | Luz principal, intensa, lateral-frontal (esquerda, acima, frente) |
| **Fill Light** | Luz de preenchimento, suave, lado oposto à key light |
| **Back Light** | Luz de fundo, atrás e acima do objeto — separa o objeto do fundo |

As 3 posições de luz são calculadas automaticamente com base na posição e escala do objeto principal.

## Compilação

```bash
cd build
make Vivencial2
./Vivencial2
```

## Controles

| Tecla | Ação |
|---|---|
| `X` | Rotacionar no eixo X |
| `Y` | Rotacionar no eixo Y |
| `Z` | Rotacionar no eixo Z |
| `TAB` | Selecionar próximo objeto |
| `N` | Adicionar objeto |
| `B` | Remover objeto selecionado |
| `Setas` | Mover objeto selecionado (X/Z) |
| `I / K` | Mover objeto selecionado (Y) |
| `[ / ]` | Diminuir / aumentar escala |
| `ESC` | Sair |
