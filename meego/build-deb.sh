#!/bin/sh
# Packs the ARM build as a Harmattan .deb. Runs on the build machine after
# "meego/build.sh arm":
#
#   meego/build-deb.sh                 # -> build/meego/harbour-passviewer_<version>_armel.deb
#   VERSION=1.7-meego2 meego/build-deb.sh
#
# Layout on the device:
#   /opt/harbour-passviewer/bin/harbour-passviewer
#   /opt/harbour-passviewer/qml                    (meego/qml plus qml/lib/currencies.json)
#   /opt/harbour-passviewer/translations
#   /usr/share/applications/harbour-passviewer.desktop
#   /usr/share/icons/hicolor/80x80/apps/harbour-passviewer.png
#
# mkdeb.py writes the .deb itself: Harmattan's dpkg is 1.15 and wants gzip
# members and no slash on the ar names, which GNU ar does differently.
set -e

HERE=$(cd "$(dirname "$0")/.." && pwd)
PKG=$HERE/meego
OUT=$HERE/build/meego
BIN=$OUT/arm/harbour-passviewer
XGCC=${XGCC:-/tmp/xgcc-harmattan}
VERSION=${VERSION:-$(sh "$PKG/version.sh")}

[ -x "$BIN" ] || { echo "ARM binary missing: $BIN (run meego/build.sh arm first)" >&2; exit 1; }
[ -f "$OUT/arm/translations/harbour-passviewer-de.qm" ] || { echo "translations missing in $OUT/arm" >&2; exit 1; }

STAGE=$OUT/stage
rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" "$STAGE/opt/harbour-passviewer/bin" \
         "$STAGE/usr/share/applications" "$STAGE/usr/share/icons/hicolor/80x80/apps" \
         "$STAGE/usr/share/themes/base/meegotouch/icons" "$STAGE/usr/share/doc/harbour-passviewer"

cp "$BIN" "$STAGE/opt/harbour-passviewer/bin/harbour-passviewer"
"$XGCC/bin/arm-none-linux-gnueabi-strip" "$STAGE/opt/harbour-passviewer/bin/harbour-passviewer"
chmod 755 "$STAGE/opt/harbour-passviewer/bin/harbour-passviewer"
# The HTTPS helper (meego/fetch, Rust, static): PassHandler runs it for a
# pass update, because Qt 4's OpenSSL 0.9.8 cannot reach today's servers.
FETCH=$OUT/fetch/passviewer-fetch
[ -x "$FETCH" ] || { echo "passviewer-fetch missing: $FETCH (run meego/build-rust.sh first)" >&2; exit 1; }
cp "$FETCH" "$STAGE/opt/harbour-passviewer/bin/passviewer-fetch"
chmod 755 "$STAGE/opt/harbour-passviewer/bin/passviewer-fetch"

cp -a "$PKG/qml" "$STAGE/opt/harbour-passviewer/qml"
# CurrencyFormat reads qml/lib/currencies.json through SailfishApp::pathTo().
cp "$HERE/qml/lib/currencies.json" "$STAGE/opt/harbour-passviewer/qml/lib/currencies.json"
cp -a "$OUT/arm/translations" "$STAGE/opt/harbour-passviewer/translations"

# Icons: 80x80 for the launcher, 64x64 base64 for the application manager.
# meego/icons/make-icon.py cuts both to the exact silhouette of the stock
# apps. The launcher looks in hicolor.
cp "$PKG/icons/icon-80.png" "$STAGE/usr/share/icons/hicolor/80x80/apps/harbour-passviewer.png"
cp "$PKG/icons/icon-80.png" "$STAGE/usr/share/themes/base/meegotouch/icons/harbour-passviewer-80.png"
cp "$PKG/harbour-passviewer.desktop" "$STAGE/usr/share/applications/harbour-passviewer.desktop"
cp "$HERE/LICENSE" "$STAGE/usr/share/doc/harbour-passviewer/copyright"
find "$STAGE" -type f ! -path "*/bin/*" -exec chmod 644 {} +
find "$STAGE" -type d -exec chmod 755 {} +

# control with the icon; the base64 lines need a leading space each.
VERSION="$VERSION" ICON="$PKG/icons/icon-64.png" python3 - "$PKG/control.in" "$STAGE/DEBIAN/control" <<'PY'
import base64, os, sys, textwrap
src, dst = sys.argv[1], sys.argv[2]
ctl = open(src).read()
b64 = base64.b64encode(open(os.environ["ICON"], "rb").read()).decode("ascii")
icon = "\n".join(" " + line for line in textwrap.wrap(b64, 76))
ctl = ctl.replace("@VERSION@", os.environ["VERSION"]).replace("@ICON@", icon)
open(dst, "w").write(ctl)
PY

DEB="$OUT/harbour-passviewer_${VERSION}_armel.deb"
python3 "$PKG/mkdeb.py" "$STAGE" "$DEB"
echo "== $DEB"
