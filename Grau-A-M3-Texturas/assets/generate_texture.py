#!/usr/bin/env python3
# Gera uma textura TGA 256x256 com padro xadrez colorido

import struct

WIDTH = 256
HEIGHT = 256
CHECK_SIZE = 32

pixels = bytearray()
for y in range(HEIGHT):
    for x in range(WIDTH):
        cx = x // CHECK_SIZE
        cy = y // CHECK_SIZE
        if (cx + cy) % 2 == 0:
            # Laranja
            b, g, r = 0, 128, 255
        else:
            # Roxo
            b, g, r = 128, 64, 128
        pixels.extend([b, g, r])

header = bytearray([
    0,          # ID length
    0,          # Color map type
    2,          # Image type: uncompressed true-color
    0, 0,       # Color map first entry
    0, 0,       # Color map length
    0,          # Color map entry size
    0, 0,       # X origin
    0, 0,       # Y origin
    WIDTH & 0xFF, (WIDTH >> 8) & 0xFF,   # Width
    HEIGHT & 0xFF, (HEIGHT >> 8) & 0xFF, # Height
    24,         # Bits per pixel
    0x20        # Image descriptor: top-left origin
])

import os
out_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "texture.tga")

with open(out_path, "wb") as f:
    f.write(header)
    f.write(pixels)

print("Textura " + out_path + " gerada com sucesso.")
