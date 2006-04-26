#!/bin/sh

ROOT_DIR="$1"
PACKAGE_DIR="$2"

cd "$ROOT_DIR"
/usr/bin/tar xzf "$PACKAGE_DIR/KisMAC.tgz"

/usr/bin/touch "$ROOT_DIR"