#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Builds the launcher icon in MeeGo's own icon shape.

Harmattan launcher icons are not free-form: every stock icon is cut to the
same rounded-square "squircle", and an icon that keeps its own outline -- a
plain square, as the Sailfish one is -- reads as foreign among them on the
home screen. The silhouette is therefore taken literally from a stock icon's
alpha channel rather than approximated: the icons under
/usr/share/themes/blanco/meegotouch/icons/icon-l-*.png all carry it, pixel
for pixel, and one of them is kept here beside the script as mask-icon-l.png.

The artwork itself is Pass Viewer's own, straight from the Sailfish icon -- only
its outline changes. Everything is done at 4x and scaled down at the end;
the curve aliases badly if it is drawn at 80 px.

    python3 meego/icons/make-icon.py      # writes icon-80.png and icon-64.png
"""
from PIL import Image

HERE = __file__.rsplit("/", 1)[0]
MASK_SOURCE = HERE + "/mask-icon-l.png"          # a stock icon, for its alpha
ART_SOURCE = HERE + "/../../icons/172x172/harbour-passviewer.png"

S = 320                                          # working size, 4x the largest output
SIZES = (80, 64)


def squircle():
    """MeeGo's icon silhouette, straight out of a stock icon's alpha."""
    stock = Image.open(MASK_SOURCE).convert("RGBA")
    return stock.split()[3].resize((S, S), Image.LANCZOS)


def build():
    art = Image.open(ART_SOURCE).convert("RGBA").resize((S, S), Image.LANCZOS)
    icon = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    # The Sailfish icon fills its square edge to edge, so the mask alone
    # decides the outline -- its own faint corner rounding disappears well
    # inside the cut.
    icon.paste(art, (0, 0))
    icon.putalpha(squircle())
    for size in SIZES:
        icon.resize((size, size), Image.LANCZOS).save("%s/icon-%d.png" % (HERE, size))
        print("icon-%d.png" % size)


if __name__ == "__main__":
    build()
