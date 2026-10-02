#!/bin/sh
# The version the MeeGo build stamps into binary and package: the Sailfish
# release this port follows (rpm/harbour-passviewer.yaml) plus its own
# revision.
HERE=$(cd "$(dirname "$0")/.." && pwd)
UP=$(sed -n 's/^Version: *//p' "$HERE/rpm/harbour-passviewer.yaml" | head -1)
echo "${UP:-1.7}-meego2"
