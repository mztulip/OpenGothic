#!/usr/bin/env bash

BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

DIR="$BUILD_DIR/build/opengothic"


GAME=/home/mz/.wine/drive_c/Program\ Files\ \(x86\)/Piranha\ Bytes/Gothic/

cd "$DIR" || exit 1

export LD_LIBRARY_PATH="$DIR:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="$DIR:$DYLD_LIBRARY_PATH"

exec "$DIR/Gothic2Notr" \
    -g "$GAME" \
    -devmode -g1 -rtsm 1
