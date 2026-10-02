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
its outline changes. The cut and everything drawn with it happen at 4x and are
scaled down at the end; the curve aliases badly if it is drawn at 80 px. Only
the seam below is read and painted out in the artwork's own 172 px, where its
stitches are still sharp.

Cutting alone is not enough, though: the wallet's stitching runs along the top
and left edge at a constant inset, so it follows the outline it was drawn for
-- the Sailfish square. Cut to the squircle it keeps running straight into the
rounded corner and is chopped off there. The seam is therefore lifted off the
artwork and laid down again along the squircle: the old stitches are measured
(where they are, how dark they are against what they lie on), painted out, and
redrawn on a path that is the silhouette itself, moved inward by the inset they
had. Their darkness is carried over from the measurement, so the seam keeps
fading out where the artwork turns into shadow and keeps its gap where the
boarding pass covers the edge -- the stitching is never invented, only moved.

    python3 meego/icons/make-icon.py      # writes icon-80.png and icon-64.png
"""
import math

import numpy as np
from PIL import Image, ImageDraw

HERE = __file__.rsplit("/", 1)[0]
MASK_SOURCE = HERE + "/mask-icon-l.png"          # a stock icon, for its alpha
ART_SOURCE = HERE + "/../../icons/172x172/harbour-passviewer.png"

S = 320                                          # working size, 4x the largest output
SIZES = (80, 64)

# The seam, measured on the Sailfish icon, in its own 172 px pixels.
ART = 172
SEAM_INSET = 8.0            # centre line, from the edge of the artwork
SEAM_BAND = (5, 11)         # rows/columns the stitches live in
SEAM_FLANKS = (4, 12)       # untouched rows/columns just outside them
SEAM_WIDTH = 2.6            # stitch thickness
DASH, GAP = 6.0, 4.0        # stitch, then gap
SEAM_MIN_DELTA = 10         # darker than its ground by this much, or it is not a stitch
SEAM_EVEN_GROUND = 20       # and the ground to both sides of it must agree this closely
SEAM_REACH = 6              # half a dash period, for closing the gaps when measuring
SEAM_SMOOTH = 8             # and for letting the measurement fade instead of step
RAYS = 4096                 # how finely the silhouette's outline is walked
TANGENT = 16                # rays apart, for reading the direction of that outline


def squircle():
    """MeeGo's icon silhouette, straight out of a stock icon's alpha."""
    stock = Image.open(MASK_SOURCE).convert("RGBA")
    return stock.split()[3].resize((S, S), Image.LANCZOS)


def _luma(rgb):
    return rgb @ np.array([0.299, 0.587, 0.114])


def lift_seam(art):
    """Paints the old stitches out and reports how dark each of them was.

    Only the top and the left edge are looked at: those are the two the wallet
    is stitched along. The right and the bottom are the shadowed, rolled edge,
    where a brightness test would find darkness everywhere and read it as
    stitching that was never there.

    A stitch is a thin grey line on even ground, so each pixel of the band is
    held against the ground interpolated across it from the two untouched
    flanks; what is markedly darker, and grey, and has the same ground on both
    sides of it, is a stitch and gets the ground instead. That last condition
    is what keeps the shadow out: towards the top right corner the wallet darkens
    steeply across the band, which on its own reads exactly like a very broad
    stitch. It also keeps the gap where the boarding pass lies over the edge.

    The measured drops are returned per column and per row, closed over the gaps
    between the stitches and smoothed, which is what later makes the new seam
    fade where the old one faded.
    """
    rgb = np.asarray(art.convert("RGB"), dtype=float)
    lo, hi = SEAM_BAND
    near, far = SEAM_FLANKS
    drop = {}
    for edge in ("top", "left"):
        # Work on the top edge; for the left one the image is simply transposed.
        band = rgb if edge == "top" else rgb.transpose(1, 0, 2)
        depth = np.arange(lo, hi + 1, dtype=float)[:, None, None]
        t = (depth - near) / float(far - near)
        ground = band[near] * (1.0 - t) + band[far] * t      # the ground under the seam
        here = band[lo:hi + 1]
        fall = _luma(ground) - _luma(here)
        grey = (here.max(2) - here.min(2) <= 12) & (ground.max(2) - ground.min(2) <= 12)
        even = abs(_luma(band[near]) - _luma(band[far])) <= SEAM_EVEN_GROUND
        stitch = (fall >= SEAM_MIN_DELTA) & grey & even
        band[lo:hi + 1] = np.where(stitch[..., None], ground, here)
        drop[edge] = np.where(stitch, fall, 0.0).max(0)       # deepest drop per column
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8))
    out.putalpha(art.split()[3])
    return out, {e: _envelope(d) for e, d in drop.items()}


def _envelope(drop):
    """Closes the gaps between the stitches, then smooths what is left."""
    pad = np.pad(drop, SEAM_REACH, mode="edge")
    closed = np.array([pad[i:i + 2 * SEAM_REACH + 1].max() for i in range(len(drop))])
    pad = np.pad(closed, SEAM_SMOOTH, mode="edge")
    window = 2 * SEAM_SMOOTH + 1
    return np.convolve(pad, np.ones(window) / window, mode="valid")


def seam_path(alpha, inset):
    """The silhouette's own outline, moved inward by `inset`."""
    a = np.asarray(alpha, dtype=float)
    cy, cx = (np.array(a.shape) - 1) / 2.0
    ang = np.linspace(0.0, 2.0 * math.pi, RAYS, endpoint=False)
    ux, uy = np.cos(ang), np.sin(ang)
    radii = np.arange(0.0, S / 2.0, 0.25)[:, None]
    v = _sample(a, cx + radii * ux, cy + radii * uy)
    # Walk every ray outward to where the silhouette ends, and read that crossing
    # off between the two samples around it: whole samples leave the outline
    # notched, and a notched outline has no usable direction.
    last = np.argmax(np.cumsum(v >= 128, 0) == (v >= 128).sum(0), 0)
    lo = v[last, np.arange(RAYS)]
    hi = v[np.minimum(last + 1, len(radii) - 1), np.arange(RAYS)]
    frac = np.where(lo > hi, (lo - 128.0) / np.maximum(lo - hi, 1e-6), 0.0)
    edge = radii[:, 0][last] + 0.25 * np.clip(frac, 0.0, 1.0)
    px, py = cx + edge * ux, cy + edge * uy
    # Inward normal, from the direction the outline runs in a few rays either way.
    tx = np.roll(px, -TANGENT) - np.roll(px, TANGENT)
    ty = np.roll(py, -TANGENT) - np.roll(py, TANGENT)
    nx, ny = ty, -tx
    length = np.hypot(nx, ny)
    nx, ny = nx / length, ny / length
    if (nx * (cx - px) + ny * (cy - py)).sum() < 0:
        nx, ny = -nx, -ny
    return px + inset * nx, py + inset * ny, nx, ny


def _sample(a, x, y):
    """Reads the image between its pixels."""
    x = np.clip(x, 0, a.shape[1] - 1.001)
    y = np.clip(y, 0, a.shape[0] - 1.001)
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    fx, fy = x - x0, y - y0
    return ((a[y0, x0] * (1 - fx) + a[y0, x0 + 1] * fx) * (1 - fy)
            + (a[y0 + 1, x0] * (1 - fx) + a[y0 + 1, x0 + 1] * fx) * fy)


def stitch_depth(x, y, nx, ny, drop):
    """How dark a stitch at this point was, where the old seam ran."""
    ax, ay = x * ART / float(S), y * ART / float(S)
    wx, wy = abs(nx) / (abs(nx) + abs(ny)), abs(ny) / (abs(nx) + abs(ny))
    def look(table, at):
        at = int(round(at))
        return table[at] if 0 <= at < len(table) else 0.0
    # The normal points inward, so the top edge is the one whose normal points down.
    down = wy * look(drop["top"], ax) if ny > 0 else 0.0
    right = wx * look(drop["left"], ay) if nx > 0 else 0.0
    return down + right


def lay_seam(art, alpha, drop):
    """Draws the lifted seam again, along the silhouette."""
    scale = S / float(ART)
    sx, sy, nx, ny = seam_path(alpha, SEAM_INSET * scale)
    # Begin at the top left corner and centre a stitch on it, so the corner
    # carries its stitch around the bend the way the drawn one does.
    start = int(np.argmin((sx - sx.min()) ** 2 + (sy - sy.min()) ** 2))
    order = np.roll(np.arange(len(sx)), -start)
    sx, sy, nx, ny = sx[order], sy[order], nx[order], ny[order]
    walk = np.concatenate(([0.0], np.cumsum(np.hypot(np.diff(sx), np.diff(sy)))))
    period = (DASH + GAP) * scale
    on = np.mod(walk - DASH * scale / 2.0, period) < DASH * scale
    over = 2                                     # supersample the stitches once more
    shade = Image.new("L", (S * over, S * over), 0)
    pen = ImageDraw.Draw(shade)
    width = max(1, int(round(SEAM_WIDTH * scale * over)))
    run = []
    for i in range(len(sx) + 1):
        if i < len(sx) and on[i]:
            run.append(i)
            continue
        if len(run) > 1:
            depth = np.mean([stitch_depth(sx[j], sy[j], nx[j], ny[j], drop) for j in run])
            if depth >= 1.0:
                pen.line([(sx[j] * over, sy[j] * over) for j in run],
                         fill=int(round(depth)), width=width, joint="curve")
        run = []
    shade = np.asarray(shade.resize((S, S), Image.BOX), dtype=float)
    rgb = np.asarray(art.convert("RGB"), dtype=float) - shade[..., None]
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8))
    out.putalpha(art.split()[3])
    return out


def build():
    # The stitches are lifted where they are sharp, in the artwork's own pixels;
    # everything after that happens at 4x.
    art, drop = lift_seam(Image.open(ART_SOURCE).convert("RGBA"))
    art = art.resize((S, S), Image.LANCZOS)
    icon = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    # The Sailfish icon fills its square edge to edge, so the mask alone
    # decides the outline -- its own faint corner rounding disappears well
    # inside the cut.
    icon.paste(art, (0, 0))
    mask = squircle()
    icon.putalpha(mask)
    icon = lay_seam(icon, mask, drop)
    for size in SIZES:
        icon.resize((size, size), Image.LANCZOS).save("%s/icon-%d.png" % (HERE, size))
        print("icon-%d.png" % size)


if __name__ == "__main__":
    build()
