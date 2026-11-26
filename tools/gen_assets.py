#!/usr/bin/env python3
"""Regenerates src/assets.cpp from assets/*.png and the glyph table below.
The generated file is committed, so the ROM build does not need Python."""
import os
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLAYER_BASE, SHARD_BASE = 64, 80

# Glyphs measured from assets/font_reference.png (8px cells, 5 rows tall).
# D E S T I N Y are copied exactly; the rest follow the same construction.
G = {
 'A': [".##.", "#..#", "####", "#..#", "#..#"],
 'B': ["###.", "#..#", "###.", "#..#", "###."],
 'C': [".###", "#...", "#...", "#...", ".###"],
 'D': ["###.", "#..#", "#..#", "#..#", "###."],
 'E': ["####", "#...", "###.", "#...", "####"],
 'F': ["####", "#...", "###.", "#...", "#..."],
 'G': [".###", "#...", "#.##", "#..#", ".###"],
 'H': ["#..#", "#..#", "####", "#..#", "#..#"],
 'I': ["#", "#", "#", "#", "#"],
 'J': ["...#", "...#", "...#", "#..#", ".##."],
 'K': ["#..#", "#.#.", "##..", "#.#.", "#..#"],
 'L': ["#...", "#...", "#...", "#...", "####"],
 'M': ["#...#", "##.##", "#.#.#", "#...#", "#...#"],
 'N': ["#...#", "##..#", "#.#.#", "#..##", "#...#"],
 'O': [".##.", "#..#", "#..#", "#..#", ".##."],
 'P': ["###.", "#..#", "###.", "#...", "#..."],
 'Q': [".##.", "#..#", "#..#", "#.##", ".###"],
 'R': ["###.", "#..#", "###.", "#.#.", "#..#"],
 'S': [".###", "#...", ".##.", "...#", "###."],
 'T': ["#####", "..#..", "..#..", "..#..", "..#.."],
 'U': ["#..#", "#..#", "#..#", "#..#", ".##."],
 'V': ["#...#", "#...#", "#...#", ".#.#.", "..#.."],
 'W': ["#...#", "#...#", "#.#.#", "##.##", "#...#"],
 'X': ["#...#", ".#.#.", "..#..", ".#.#.", "#...#"],
 'Y': ["#...#", "#...#", ".###.", "..#..", "..#.."],
 'Z': ["####", "...#", ".##.", "#...", "####"],
 '0': [".##.", "#.##", "#..#", "##.#", ".##."],
 '1': [".#.", "##.", ".#.", ".#.", "###"],
 '2': [".##.", "#..#", "..#.", ".#..", "####"],
 '3': ["###.", "...#", ".##.", "...#", "###."],
 '4': ["#..#", "#..#", "####", "...#", "...#"],
 '5': ["####", "#...", "###.", "...#", "###."],
 '6': [".###", "#...", "###.", "#..#", ".##."],
 '7': ["####", "...#", "..#.", ".#..", ".#.."],
 '8': [".##.", "#..#", ".##.", "#..#", ".##."],
 '9': [".##.", "#..#", ".###", "...#", "###."],
 ' ': ["...", "...", "...", "...", "..."],
 '!': ["#", "#", "#", ".", "#"],
 '"': ["#.#", "#.#", "...", "...", "..."],
 '#': ["#####", "#####", "#####", "#####", "#####"],
 '$': [".###.", "#.#..", ".###.", "..#.#", ".###."],
 '%': ["#..#", "...#", "..#.", ".#..", "#..#"],
 '&': [".##.", "#...", ".#.#", "#.#.", ".#.#"],
 "'": ["#", "#", ".", ".", "."],
 '(': [".#", "#.", "#.", "#.", ".#"],
 ')': ["#.", ".#", ".#", ".#", "#."],
 '*': ["#.#", ".#.", "#.#", "...", "..."],
 '+': ["...", ".#.", "###", ".#.", "..."],
 ',': ["..", "..", "..", ".#", "#."],
 '-': ["...", "...", "###", "...", "..."],
 '.': [".", ".", ".", ".", "#"],
 '/': ["..#", "..#", ".#.", "#..", "#.."],
 ':': [".", "#", ".", "#", "."],
 ';': [".", "#", ".", "#", "#"],
 '<': ["..#", ".#.", "#..", ".#.", "..#"],
 '=': ["...", "###", "...", "###", "..."],
 '>': ["#..", ".#.", "..#", ".#.", "#.."],
 '?': [".##.", "#..#", "..#.", "....", "..#."],
 '@': [".##.", "#..#", "#.##", "#...", ".###"],
 '[': ["##", "#.", "#.", "#.", "##"],
 '\\': ["#..", "#..", ".#.", "..#", "..#"],
 ']': ["##", ".#", ".#", ".#", "##"],
 '^': [".#.", "#.#", "...", "...", "..."],
 '_': ["....", "....", "....", "....", "####"],
 '`': ["#.", ".#", "..", "..", ".."],
 '{': [".##", ".#.", "#..", ".#.", ".##"],
 '|': ["#", "#", "#", "#", "#"],
 '}': ["##.", ".#.", "..#", ".#.", "##."],
 '~': ["....", ".#..", "#.#.", "..#.", "...."],
}

KERN = {('D', 'E'): 1, ('S', 'T'): -2}   # measured from the reference image
TRACK = 4                                 # default gap in cells

def glyph_for(ch):
    if 'a' <= ch <= 'z':
        ch = ch.upper()
    return G.get(ch, G['?'])

def width(rows):
    return max(len(r) for r in rows)

def rows_bits(rows):
    out = []
    for r in rows:
        v = 0
        for i, c in enumerate(r):
            if c == '#':
                v |= 0x10 >> i
        out.append(v)
    return out

def text_cells(s):
    """Return list of (x_cell, glyph rows) laid out like the reference."""
    x, res = 0, []
    for i, ch in enumerate(s):
        g = glyph_for(ch)
        res.append((x, g))
        x += width(g)
        if i + 1 < len(s):
            x += TRACK + KERN.get((ch.upper(), s[i + 1].upper()), 0)
    return res, x

def console_tile(rows):
    w = width(rows)
    xo = (8 - w) // 2
    px = [[0] * 8 for _ in range(8)]
    for r, row in enumerate(rows):
        for c, ch in enumerate(row):
            if ch == '#':
                px[1 + r][xo + c] = 1
    words = []
    for y in range(8):
        for half in range(2):
            v = 0
            for k in range(4):
                v |= px[y][half * 4 + k] << (4 * k)
            words.append(v)
    return words

def rgb15(r, g, b):
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)

def sprite(path, base):
    im = Image.open(path).convert('RGBA')
    cols, idx, pix = [], {}, []
    for y in range(im.height):
        for x in range(im.width):
            r, g, b, a = im.getpixel((x, y))
            if a < 128:
                pix.append(0)
                continue
            k = (r, g, b)
            if k not in idx:
                idx[k] = len(cols)
                cols.append(k)
            pix.append(base + idx[k])
    return im.width, im.height, pix, [rgb15(*c) for c in cols]

def arr(name, ctype, vals, per=16, fmt="%d"):
    s = "const %s %s[%d] = {\n" % (ctype, name, len(vals))
    for i in range(0, len(vals), per):
        s += "    " + ", ".join(fmt % v for v in vals[i:i + per]) + ",\n"
    return s + "};\n\n"

def main():
    chars = [chr(c) for c in range(32, 127)]
    widths = [width(glyph_for(c)) for c in chars]
    bits = [b for c in chars for b in rows_bits(glyph_for(c))]
    tiles = [w for c in chars for w in console_tile(glyph_for(c))]
    pw, ph, ppix, ppal = sprite(os.path.join(ROOT, "assets/player_idle.png"), PLAYER_BASE)
    sw_, sh_, gpix, gpal = sprite(os.path.join(ROOT, "assets/Shard_Died.png"), SHARD_BASE)
    assert PLAYER_BASE + len(ppal) <= SHARD_BASE and SHARD_BASE + len(gpal) <= 256
    out = "// Generated by tools/gen_assets.py - do not edit by hand.\n#include \"assets.h\"\n\n"
    out += arr("kFontW", "u8", widths, 19)
    out += "const u8 kFontRows[95][5] = {\n"
    for i in range(0, len(bits), 5):
        out += "    {" + ", ".join("0x%02X" % v for v in bits[i:i + 5]) + "},\n"
    out += "};\n\n"
    out += arr("kConsoleFont", "u16", tiles, 8, "0x%04X").replace("const u16", "const u16 __attribute__((aligned(4)))", 1)
    out += "const u16 kConsolePal[16] = {0x0000, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF};\n\n"
    out += arr("kPlayerIdle", "u8", ppix, 32)
    out += arr("kPlayerPal", "u16", ppal, 8, "0x%04X")
    out += arr("kShardDied", "u8", gpix, 32)
    out += arr("kShardPal", "u16", gpal, 8, "0x%04X")
    open(os.path.join(ROOT, "src/assets.cpp"), "w").write(out)
    print("player colors", len(ppal), "shard colors", len(gpal))

if __name__ == "__main__":
    main()
