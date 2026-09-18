#!/usr/bin/env bash

# Katalog, w którym znajduje się ten skrypt
BUILD_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Katalog z executable OpenGothic
DIR="$BUILD_DIR/build/opengothic"

# Instalacja Gothic 2 używana jako źródło danych
GAME="/home/mz/.wine/drive_c/Program Files (x86)/JoWood/Gothic II"

cd "$DIR" || exit 1

export LD_LIBRARY_PATH="$DIR:$LD_LIBRARY_PATH"
export DYLD_LIBRARY_PATH="$DIR:$DYLD_LIBRARY_PATH"

exec "$DIR/Gothic2Notr" \
    -g "$GAME" \
    -devmode -gi 1 -rtsm 1 -saves ../../light_test_saves
