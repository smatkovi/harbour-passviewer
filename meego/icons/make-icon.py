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

Cutting alone is not enough, though. Two things of the artwork still belong to
the square it was drawn in.

The first is the material. The wallet is seen at an angle and its own rounded
edge leaves the top right corner of the image empty; the squircle reaches
further out there than the wallet does, and stamping the silhouette on as the
alpha channel would turn that empty corner into a black crescent -- the shape
cut out, but nothing filling it. The leather is therefore grown outward into
whatever the silhouette covers and the artwork does not.

The second is the stitching: the wallet's seam runs along the top
and left edge at a constant inset, so it follows the outline it was drawn for
-- the Sailfish square. Cut to the squircle it keeps running straight into the
rounded corner and is chopped off there. The seam is therefore lifted off the
artwork and laid down again along the squircle: the old stitches are measured,
painted out, and redrawn on a path that is the silhouette itself, moved inward
by the inset they had.

The new seam runs the whole way round, which the drawn one never did -- it
stopped where the wallet turns into shadow, because on the Sailfish square that
shadowed side is a corner one hardly looks at, and here it is a full flank of
the icon. What the measurement hands over for that is not a darkness but a
fraction: a stitch is about four tenths darker than whatever it lies on, lit
back or shadowed edge alike. The one place it stays away from is the boarding
pass -- where the pass covers the edge of the wallet, the stitching is behind
it, and the gap the artwork shows there is kept.

    python3 meego/icons/make-icon.py      # writes icon-80.png and icon-64.png
"""
import math

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

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
SEAM_FAINT = 2              # but once a run is found, this much is swept away with it
SEAM_EVEN_GROUND = 20       # and the ground to both sides of it must agree this closely
SEAM_REACH = 6              # half a dash period, for closing the gaps when measuring
SEAM_SMOOTH = 8             # and for letting the measurement fade instead of step
LEATHER_MAX = 140           # brighter than this, or
LEATHER_SAT = 25            # more coloured than this, is the boarding pass, not the wallet
LOOSE_FALL = 6              # a stitch is at least this much darker than around it
LOOSE_AREA = 60             # and is a short dash: no more pixels than this,
LOOSE_SIZE = 12             # and no longer than this in either direction
CROWD = 18                  # this close to the outline it may be a second outline,
ACROSS = 0.5                # unless it runs across it instead of along it
LOOSE_BLEED = 2             # swept this much wider, so no soft edge of it stays behind
FILL_STEPS = 48             # how far the leather may be drawn out to meet the silhouette
RAYS = 4096                 # how finely the silhouette's outline is walked
TANGENT = 16                # rays apart, for reading the direction of that outline


def squircle():
    """MeeGo's icon silhouette, straight out of a stock icon's alpha."""
    stock = Image.open(MASK_SOURCE).convert("RGBA")
    return stock.split()[3].resize((S, S), Image.LANCZOS)


def fill_to_silhouette(art, alpha):
    """Draws the leather out to wherever the silhouette reaches.

    The Sailfish icon is not a filled square: the wallet is seen at an angle
    and its own rounded edge leaves the top right corner of the image empty.
    The squircle reaches further out there than the wallet does, and simply
    stamping the silhouette on as the alpha channel turns that empty corner
    into a black crescent -- the cut-out shape shows, but no material fills it.

    So the material is grown outward instead: every pixel the silhouette covers
    but the artwork does not takes the mean of its covered neighbours, one ring
    at a time, until the corner is closed. What grows out is the dark rolled
    edge that borders the gap, which is exactly the material that belongs there.
    """
    rgb = np.asarray(art.convert("RGB"), dtype=float)
    have = np.asarray(art.split()[3], dtype=float) >= 250
    want = (np.asarray(alpha, dtype=float) > 0) & ~have
    rgb = _grow_into(rgb, have, want, FILL_STEPS)
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).convert("RGBA")
    out.putalpha(art.split()[3])
    return out


def _grow_into(rgb, have, want, steps):
    """Closes the wanted pixels from the ones around them, one ring at a time."""
    for _ in range(steps):
        if not want.any():
            break
        total = np.zeros_like(rgb)
        count = np.zeros(rgb.shape[:2])
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy == dx == 0:
                    continue
                total += np.roll(np.roll(rgb * have[..., None], dy, 0), dx, 1)
                count += np.roll(np.roll(have.astype(float), dy, 0), dx, 1)
        ring = want & (count > 0)
        rgb = np.where(ring[..., None], total / np.maximum(count, 1)[..., None], rgb)
        have, want = have | ring, want & ~ring
    return rgb


def _blobs(flag):
    """The connected runs of a flag, as lists of their pixels."""
    seen = np.zeros(flag.shape, bool)
    out = []
    for y, x in zip(*np.nonzero(flag)):
        if seen[y, x]:
            continue
        stack, blob = [(y, x)], []
        seen[y, x] = True
        while stack:
            a, b = stack.pop()
            blob.append((a, b))
            for c in range(max(a - 1, 0), min(a + 2, flag.shape[0])):
                for d in range(max(b - 1, 0), min(b + 2, flag.shape[1])):
                    if flag[c, d] and not seen[c, d]:
                        seen[c, d] = True
                        stack.append((c, d))
        out.append(blob)
    return out


def erase_loose_stitches(art, alpha):
    """Takes out the stitching that runs beside the seam along the outline.

    The wallet carries more than one seam. The back is stitched around its own
    rounded corner, which on the Sailfish square sits in the corner and under
    the squircle runs down the right flank, a few pixels beside the new seam
    and curving harder, because it follows a smaller shape. The pocket is
    stitched along its own flank too, but that one lies deep inside the icon
    and reads as what it is -- the pocket's line, not a second outline.

    So two things decide, and a stitch has to fail both to go: how close to the
    outline it lies, and which way it runs there. Close and alongside is a
    second outline. Close but across it is some other line of the wallet
    arriving at the edge -- the seam along the mouth of the pocket runs out to
    the left edge like that, and it has to keep running out to meet the seam
    coming down. A dash is long enough to say which way it points, and the
    direction to the nearest point of the outline says what to hold it against:
    measured here, the back's stitches come out at 0.05 to 0.20 of running
    along, the pocket's at 0.95 to 1.00 of running across.

    A stitch is told from the outlines it runs beside by being short: those are
    long unbroken curves, a dash is a dozen pixels. And it is swept a little
    wider than it is found, or its soft edge stays behind as a ghost of the line.
    """
    rgb = np.asarray(art.convert("RGB"), dtype=float)
    lit = Image.fromarray(np.clip(_luma(rgb), 0, 255).astype(np.uint8))
    ground = np.asarray(lit.filter(ImageFilter.MaxFilter(5)), dtype=float)
    around = np.dstack([np.asarray(Image.fromarray(np.clip(rgb[:, :, c], 0, 255)
                                                  .astype(np.uint8))
                                   .filter(ImageFilter.MaxFilter(5)), dtype=float)
                        for c in range(3)])
    leather = ((rgb.max(2) - rgb.min(2) <= LEATHER_SAT)
               & (around.max(2) - around.min(2) <= LEATHER_SAT)
               & (ground <= LEATHER_MAX))
    loose = (ground - _luma(rgb) >= LOOSE_FALL) & leather
    inside = np.asarray(alpha.resize((ART, ART), Image.LANCZOS), dtype=float) > 128
    rim = (inside ^ np.roll(inside, 1, 0)) | (inside ^ np.roll(inside, 1, 1))
    ry, rx = np.nonzero(rim)
    want = np.zeros(loose.shape, bool)
    for blob in _blobs(loose):
        ys = np.array([a for a, b in blob], dtype=float)
        xs = np.array([b for a, b in blob], dtype=float)
        if len(blob) > LOOSE_AREA:
            continue
        if max(ys.max() - ys.min(), xs.max() - xs.min()) > LOOSE_SIZE:
            continue
        cy, cx = np.mean(ys), np.mean(xs)
        reach = np.hypot(ry - cy, rx - cx)
        near = reach.min()
        if near > CROWD:
            continue                                   # the wallet's own inner line
        if len(blob) >= 4:
            out = np.argmin(reach)
            way = np.array([ry[out] - cy, rx[out] - cx]) / max(near, 1e-6)
            run = np.linalg.eigh(np.cov(np.stack([ys - cy, xs - cx])))[1][:, -1]
            if abs(float(run @ way)) > ACROSS:
                continue                               # it crosses the outline, not along
        for a, b in blob:
            want[a, b] = True
    for _ in range(LOOSE_BLEED):
        wider = want.copy()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                wider |= np.roll(np.roll(want, dy, 0), dx, 1)
        want = wider & leather
    rgb = _grow_into(rgb, ~want, want, 8)
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).convert("RGBA")
    out.putalpha(art.split()[3])
    return out


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

    That strict test finds where the seam runs, but not all of it: where the
    stitching fades towards the shadow it drops below the threshold, and its
    soft edges always do. A second sweep therefore takes the band down to its
    ground wherever the first one found a run at all -- otherwise the old
    straight line stays standing next to the new curved one, faint but visible.

    What is returned is how dark a stitch is against what it lies on, as a
    fraction: the drops and their grounds vary all over the artwork, but their
    ratio hardly does, and a fraction is the one reading that can be carried
    anywhere -- including the shadowed side, where the wallet was never drawn
    with a seam at all.
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
        run = _envelope(np.where(stitch, fall, 0.0).max(0))          # deepest drop per column
        swept = grey & even & (fall >= SEAM_FAINT) & (run > SEAM_FAINT)
        band[lo:hi + 1] = np.where(swept[..., None], ground, here)
        # The deepest point of each stitch, against the ground right there: the
        # soft edges of a stitch are shallower and would talk the fraction down.
        deep = np.where(stitch, fall, 0.0).argmax(0)
        col = np.arange(fall.shape[1])
        drop[edge] = (fall[deep, col], _luma(ground)[deep, col], stitch.any(0))
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8))
    out.putalpha(art.split()[3])
    fall = np.concatenate([d[0][d[2]] for d in drop.values()])
    ground = np.concatenate([d[1][d[2]] for d in drop.values()])
    return out, float(np.median(fall / np.maximum(ground, 1.0)))


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
    # Far enough to leave the silhouette on every ray: its corners lie further
    # from the centre than half the image is wide, and a search that stops at
    # half the width reports the limit itself as the outline -- a circular arc
    # cutting across all four corners.
    radii = np.arange(0.0, S / math.sqrt(2.0) + 1.0, 0.25)[:, None]
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


def stitch_shade(rgb, points, share):
    """How dark a stitch is here, and whether one belongs here at all.

    A stitch is drawn as a fraction of the ground it lies on, so it keeps the
    same weight on the lit back of the wallet and on the shadowed rolled edge,
    where nothing was ever drawn. What it must not be drawn on is the boarding
    pass: where the pass covers the edge of the wallet, the stitching is behind
    it, and the artwork shows that gap. Leather is grey and mid-dark, the pass
    is white or red, which is all the telling apart that is needed.
    """
    patch = np.array([rgb[int(round(y)), int(round(x))] for x, y in points])
    ground = float(np.mean(_luma(patch)))
    colour = float(np.mean(patch.max(1) - patch.min(1)))
    if ground > LEATHER_MAX or colour > LEATHER_SAT:
        return 0.0
    return share * ground


def lay_seam(art, alpha, share):
    """Draws the lifted seam again, along the silhouette -- all the way round."""
    rgb = np.asarray(art.convert("RGB"), dtype=float)
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
            depth = stitch_shade(rgb, [(sx[j], sy[j]) for j in run], share)
            if depth >= 1.0:
                pen.line([(sx[j] * over, sy[j] * over) for j in run],
                         fill=int(round(depth)), width=width, joint="curve")
        run = []
    shade = np.asarray(shade.resize((S, S), Image.BOX), dtype=float)
    rgb = rgb - shade[..., None]
    out = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8))
    out.putalpha(art.split()[3])
    return out


def build():
    # The stitches are lifted where they are sharp, in the artwork's own pixels;
    # everything after that happens at 4x.
    art, share = lift_seam(Image.open(ART_SOURCE).convert("RGBA"))
    mask = squircle()
    art = erase_loose_stitches(art, mask)
    art = art.resize((S, S), Image.LANCZOS)
    icon = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    icon.paste(art, (0, 0))
    icon = fill_to_silhouette(icon, mask)
    icon.putalpha(mask)
    icon = lay_seam(icon, mask, share)
    for size in SIZES:
        icon.resize((size, size), Image.LANCZOS).save("%s/icon-%d.png" % (HERE, size))
        print("icon-%d.png" % size)


if __name__ == "__main__":
    build()
