#!/bin/sh
# Builds the MeeGo Harmattan edition (Nokia N9 / N950) on the build machine,
# inside the synchronised source tree:
#
#   meego/build.sh arm     the device binary -> build/meego/arm/harbour-passviewer
#   meego/build.sh check   the QML checker (meego/tests/qml_check.cpp) against
#                          the SDK's desktop Qt 4; meego/tests/check-qml.sh runs it
#
# The ARM build needs the GCC 14 cross toolchain (XGCC, see
# harbour-snapszer/meego/toolchain.sh) and the MADDE sysroot (SYSROOT); moc
# and lrelease come from the Qt Simulator's Qt 4.7.4, the version on the
# device. libstdc++ is linked statically and kept private, so Qt on the
# device stays on its own GCC 4.4 runtime.
set -e

HERE=$(cd "$(dirname "$0")/.." && pwd)
MODE=${1:-arm}
XGCC=${XGCC:-/tmp/xgcc-harmattan}
SYSROOT=${SYSROOT:-$HOME/QtSDK/Madde/sysroots/harmattan_sysroot_10.2011.34-1_slim}
SIMQT=${SIMQT:-$HOME/QtSDK/Simulator/Qt/gcc}
JOBS=${JOBS:-8}
VERSION=${VERSION:-$(sh "$HERE/meego/version.sh")}
OUT=$HERE/build/meego/$MODE
mkdir -p "$OUT"

# src/harbour-passviewer.cpp (SailfishApp) is replaced by meego/main.cpp and
# src/notificator.cpp (nemonotifications) by meego/notificator.cpp.
APP_SRC="src/barcodeimageprovider.cpp \
 src/homewatcher.cpp \
 src/settingsstore.cpp \
 src/zipfile.cpp \
 src/zipfileimageprovider.cpp \
 src/datetimeformat.cpp \
 src/currencyformat.cpp \
 src/passhandler.cpp \
 src/homescanner.cpp \
 src/passdb.cpp \
 src/passinfo.cpp \
 meego/main.cpp \
 meego/notificator.cpp \
 meego/platform.cpp \
 meego/compat/qt4json.cpp \
 meego/compat/sailfishapp.cpp"

# The barcode library, C.
ZINT_SRC="src/zint/qr.c src/zint/common.c src/zint/reedsol.c src/zint/aztec.c \
 src/zint/pdf417.c src/zint/large.c src/zint/library.c src/zint/bmp.c \
 src/zint/gs1.c src/zint/code128.c"

# Headers with a Q_OBJECT in them.
MOC_HEADERS="src/homewatcher.h \
 src/settingsstore.h \
 src/zipfile.h \
 src/datetimeformat.h \
 src/currencyformat.h \
 src/passhandler.h \
 src/homescanner.h \
 src/passdb.h \
 src/passinfo.h \
 meego/notificator.h \
 meego/platform.h"

INCLUDES="-I$HERE -I$HERE/src -I$HERE/meego -I$HERE/meego/compat -I$HERE/meego/compat/thirdparty"

DEFINES="-DAPP_VERSION='\"$VERSION\"' -DQT_NO_DEBUG -DNO_PNG"

# -Wno-register: the Qt 4.7 headers are older than the language the rest is
# compiled in.
COMMON_FLAGS="-O2 -Wall -Wno-register -Wno-deprecated-declarations \
 -Wno-unused-parameter $DEFINES $INCLUDES"
CXX_ONLY="-std=gnu++11 -include $HERE/meego/compat/qt4compat.h"

QT4_MODULES="QtCore QtGui QtNetwork QtSql QtDBus QtDeclarative"

case "$MODE" in
arm)
    CXX=$XGCC/bin/arm-none-linux-gnueabi-g++
    CC=$XGCC/bin/arm-none-linux-gnueabi-gcc
    [ -x "$CXX" ] || { echo "cross compiler missing: $CXX" >&2; exit 1; }
    MOC=$SIMQT/bin/moc
    QTINC=$SYSROOT/usr/include/qt4
    CFLAGS="--sysroot=$SYSROOT $COMMON_FLAGS"
    CXXFLAGS="--sysroot=$SYSROOT $COMMON_FLAGS $CXX_ONLY -I$QTINC -I$SYSROOT/usr/include/meegotouch"
    for m in $QT4_MODULES; do CXXFLAGS="$CXXFLAGS -I$QTINC/$m"; done
    # Harmattan is hard-float but kept the old loader name; the static
    # libstdc++ of GCC 14 stays private to the binary.
    LDFLAGS="--sysroot=$SYSROOT -static-libstdc++ -static-libgcc -Wl,-O1 -Wl,--as-needed \
 -Wl,--exclude-libs,ALL -Wl,--dynamic-linker=/lib/ld-linux.so.3"
    # bzip2 and lzma have no .so link and no headers in the sysroot: the
    # headers are vendored under meego/compat/thirdparty, the libraries are
    # named by their soname files.
    LIBS="-lQtDeclarative -lQtSql -lQtDBus -lQtGui -lQtNetwork -lQtCore -lmeegotouchcore \
 -lz $SYSROOT/usr/lib/libbz2.so.1.0 $SYSROOT/usr/lib/liblzma.so.2 -lpthread"
    MAIN_EXTRA=""
    ;;
check)
    # The QML checker is built against the plain desktop Qt of the SDK
    # (4.8.1): the Simulator's Qt (4.7.4) aborts without a display even for a
    # program without a window, and the device Qt does not run here.
    # QtDeclarative 1 reads the same QML either way, and that is the point.
    CXX=${CXX:-g++}
    CC=${CC:-gcc}
    PROBEQT=${PROBEQT:-$HOME/QtSDK/Desktop/Qt/4.8.1/gcc}
    [ -x "$PROBEQT/bin/moc" ] || { echo "SDK Qt 4.8 missing: $PROBEQT" >&2; exit 1; }
    MOC=$PROBEQT/bin/moc
    QTINC=$PROBEQT/include
    CFLAGS="$COMMON_FLAGS"
    CXXFLAGS="$COMMON_FLAGS $CXX_ONLY -I$QTINC"
    for m in $QT4_MODULES; do CXXFLAGS="$CXXFLAGS -I$QTINC/$m"; done
    LDFLAGS="-L$PROBEQT/lib -Wl,-rpath,$PROBEQT/lib"
    LIBS="-lQtDeclarative -lQtSql -lQtDBus -lQtGui -lQtNetwork -lQtCore -lz -lbz2 -llzma -lpthread"
    # The checker takes the place of main.cpp, notificator.cpp and
    # platform.cpp (meegotouch and the D-Bus glue are device things).
    APP_SRC=$(echo "$APP_SRC" | sed -e 's#meego/main.cpp##' -e 's#meego/notificator.cpp##' -e 's#meego/platform.cpp##')
    APP_SRC="$APP_SRC meego/tests/qml_check.cpp"
    MOC_HEADERS=$(echo "$MOC_HEADERS" | sed -e 's#meego/notificator.h##' -e 's#meego/platform.h##')
    ;;
*)
    echo "usage: $0 arm|check" >&2; exit 2 ;;
esac

MK=$OUT/Makefile
{
    echo "CXX=$CXX"; echo "CC=$CC"; echo "MOC=$MOC"
    echo "CFLAGS=$CFLAGS"; echo "CXXFLAGS=$CXXFLAGS"
    echo "LDFLAGS=$LDFLAGS"; echo "LIBS=$LIBS"; echo "SRC=$HERE"; echo
    objs=
    for s in $APP_SRC; do
        o=$(basename "$s" .cpp).o; objs="$objs $o"
        echo "$o: \$(SRC)/$s"; printf '\t$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@\n'
    done
    for s in $ZINT_SRC; do
        o=zint_$(basename "$s" .c).o; objs="$objs $o"
        echo "$o: \$(SRC)/$s"; printf '\t$(CC) $(CFLAGS) -MMD -MP -c $< -o $@\n'
    done
    for h in $MOC_HEADERS; do
        n=$(basename "$h" .h); objs="$objs moc_$n.o"
        echo "moc_$n.cpp: \$(SRC)/$h"; printf '\t$(MOC) -I$(SRC)/meego/compat $< -o $@\n'
        echo "moc_$n.o: moc_$n.cpp"; printf '\t$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@\n'
    done
    echo "OBJS=$objs"; echo
    echo "all: harbour-passviewer"
    echo "harbour-passviewer: \$(OBJS)"
    printf '\t$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)\n'
    # Header dependencies from the compiler's .d files -- without them a
    # changed header leaves stale objects behind (the Tarock 0.6.0 lesson).
    echo "-include \$(wildcard *.d)"
} > "$MK"

nice make -C "$OUT" -j"$JOBS" all

if [ "$MODE" = check ]; then
    echo "== built $OUT/harbour-passviewer (the QML checker)"
    exit 0
fi

# Translations. Qt 4.7's lrelease knows TS 2.0 only; the catalogues say 2.1.
LRELEASE=$SIMQT/bin/lrelease
mkdir -p "$OUT/translations"
for ts in "$HERE"/translations/harbour-passviewer-*.ts; do
    lang=$(basename "$ts" .ts | sed 's/^harbour-passviewer-//')
    sed 's/<TS version="2\.1"/<TS version="2.0"/' "$ts" > "$OUT/harbour-passviewer-$lang.ts"
    "$LRELEASE" -silent "$OUT/harbour-passviewer-$lang.ts" \
        -qm "$OUT/translations/harbour-passviewer-$lang.qm"
done

echo "== built $OUT/harbour-passviewer"
