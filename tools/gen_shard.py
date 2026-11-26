#!/usr/bin/env python3
"""Draws assets/Shard_Died.png (48x48): the player's Shard companion, cracked and dark.
Original design: a tall faceted crystal with a fading core, no shell and no eye."""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
T, UR, LR, B, LL, UL, C = (24, 3), (34, 15), (30, 34), (24, 45), (18, 34), (14, 15), (24, 23)

EDGE   = (14, 22, 30)
FACETS = [  # fan around the centre point, lit from the upper left
    ((T, UL, C), (150, 205, 220)),
    ((T, UR, C), (104, 160, 184)),
    ((UL, LL, C), (86, 136, 162)),
    ((UR, LR, C), (58, 100, 130)),
    ((LL, B, C), (52, 92, 120)),
    ((LR, B, C), (34, 64, 92)),
]
CRACK = (10, 16, 24)
CORE_OUT, CORE_IN, SPARK = (46, 70, 84), (92, 130, 150), (200, 236, 244)

def main():
    im = Image.new("RGBA", (48, 48), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    for pts, col in FACETS:
        d.polygon(pts, fill=col + (255,))
    for a, b in [(T, UR), (UR, LR), (LR, B), (B, LL), (LL, UL), (UL, T)]:
        d.line([a, b], fill=EDGE + (255,))
    for p in (UL, UR, LL, LR):                      # facet ridges
        d.line([p, C], fill=EDGE + (255,))
    d.polygon([(24, 17), (29, 23), (24, 29), (19, 23)], fill=CORE_OUT + (255,))   # dying core
    d.polygon([(24, 20), (26, 23), (24, 26), (22, 23)], fill=CORE_IN + (255,))
    d.point((24, 22), fill=SPARK + (255,))
    d.line([(22, 6), (25, 11), (22, 15), (26, 20)], fill=CRACK + (255,))           # fracture
    d.line([(23, 28), (20, 33), (24, 38), (22, 43)], fill=CRACK + (255,))
    for pts, col in [                                # broken-off splinters
        ([(6, 28), (10, 25), (11, 31)], (104, 160, 184)),
        ([(38, 24), (42, 27), (38, 31)], (58, 100, 130)),
        ([(35, 38), (40, 37), (37, 43)], (34, 64, 92)),
        ([(9, 10), (12, 7), (13, 12)], (150, 205, 220)),
    ]:
        d.polygon(pts, fill=col + (255,))
        d.line(pts + [pts[0]], fill=EDGE + (255,))
    im.save(os.path.join(ROOT, "assets/Shard_Died.png"))

if __name__ == "__main__":
    main()
