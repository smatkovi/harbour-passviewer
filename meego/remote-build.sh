#!/bin/sh
# Builds the N9 package from the phone: pushes this tree to the build machine
# (LAN address first, the ssh alias otherwise), restores the cross toolchain
# there if a reboot emptied /tmp, builds, packs and fetches the .deb into
# ~/ps/rpms/passviewer/.
#
#   meego/remote-build.sh            # build + pack
#   meego/remote-build.sh build      # the ARM binary only
#   meego/remote-build.sh check      # the QML checker only
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
if [ -n "$BUILD_HOST" ]; then HOST=$BUILD_HOST
elif ssh -o BatchMode=yes -o ConnectTimeout=4 sebastian@192.168.1.21 true 2>/dev/null; then HOST=sebastian@192.168.1.21
else HOST=arch; fi
REMOTE=/tmp/passviewer/src
TOOLCHAIN_TAR=${TOOLCHAIN_TAR:-$HOME/ps/toolchains/xgcc-harmattan-gcc14-hardfp.tar.gz}
MODE=${1:-package}

echo "== build host: $HOST"
ssh "$HOST" "mkdir -p $REMOTE"
rsync -a --partial --delete --exclude .git --exclude build "$HERE/" "$HOST:$REMOTE/"

if ! ssh "$HOST" test -x /tmp/xgcc-harmattan/bin/arm-none-linux-gnueabi-g++; then
    # arch keeps a copy of the tarball too (~/toolchains); the phone's is the fallback.
    if ssh "$HOST" test -f '$HOME/toolchains/xgcc-harmattan.tar.gz'; then
        ssh "$HOST" 'tar xzf $HOME/toolchains/xgcc-harmattan.tar.gz -C /tmp'
    else
        [ -f "$TOOLCHAIN_TAR" ] || { echo "toolchain tarball missing: $TOOLCHAIN_TAR" >&2; exit 1; }
        rsync -a --partial "$TOOLCHAIN_TAR" "$HOST:/tmp/xgcc-harmattan.tar.gz"
        ssh "$HOST" "tar xzf /tmp/xgcc-harmattan.tar.gz -C /tmp && rm /tmp/xgcc-harmattan.tar.gz"
    fi
fi

case "$MODE" in
build) ssh "$HOST" "cd $REMOTE && sh meego/build.sh arm"; exit 0 ;;
check) ssh "$HOST" "cd $REMOTE && sh meego/tests/check-qml.sh"; exit 0 ;;
esac

ssh "$HOST" "cd $REMOTE && sh meego/build-rust.sh && sh meego/build.sh arm && sh meego/build-deb.sh"
mkdir -p "$HOME/ps/rpms/passviewer"
rsync -a --partial "$HOST:$REMOTE/build/meego/harbour-passviewer_*_armel.deb" "$HOME/ps/rpms/passviewer/"
ls -la "$HOME/ps/rpms/passviewer/"
