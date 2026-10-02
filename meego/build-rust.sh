#!/bin/sh
# Builds passviewer-fetch (meego/fetch) for the N9/N950 -- statically against
# musl, on the build machine. meego/remote-build.sh runs this there.
#
#   meego/build-rust.sh     -> build/meego/fetch/passviewer-fetch
#
# Rust and the musl cross toolchain live under /tmp/rust on the build
# machine (meego/cross.env); mastodon-feed/tools/toolchain.sh sets them up
# again after a reboot.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/meego/cross.env"
[ -x "$CARGO_HOME/bin/cargo" ] || { echo "cargo missing under $CARGO_HOME (see mastodon-feed/tools/toolchain.sh)" >&2; exit 1; }
cd "$HERE/meego/fetch"
# The target directory stays out of the synchronised tree.
CARGO_TARGET_DIR=${CARGO_TARGET_DIR:-/tmp/passviewer-fetch-target}
export CARGO_TARGET_DIR
nice cargo build --release --target "$ZIEL"
mkdir -p "$HERE/build/meego/fetch"
cp "$CARGO_TARGET_DIR/$ZIEL/release/passviewer-fetch" "$HERE/build/meego/fetch/passviewer-fetch"
ls -la "$HERE/build/meego/fetch/passviewer-fetch"
