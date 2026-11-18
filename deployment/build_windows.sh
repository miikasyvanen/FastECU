#!/bin/bash
# Cross-compile a single static 32-bit FastECU.exe for Windows on Linux, using MXE.
#   qt6 -> Windows 10/11
#   qt5 -> Windows 7 (also runs on 10/11)
# Usage: deployment/build_windows.sh qt6|qt5 [MXE dir, default ~/.mxe]
# Build files go to deployment/build/win-<qt>/, the .exe to deployment/dist/.
set -euo pipefail

QTVER="${1:-qt6}"
MXE="${2:-${MXE_DIR:-$HOME/.mxe}}"
TARGET=i686-w64-mingw32.static
DEPLOY="$(cd "$(dirname "$0")" && pwd)"
SRC="$(dirname "$DEPLOY")"
VERSION="$(grep -oP 'software_version = "\K[^"]+' "$SRC/file_actions.h")"

case "$QTVER" in
    qt5) QMAKE="$MXE/usr/bin/$TARGET-qmake-qt5" ;;
    qt6) QMAKE="$MXE/usr/bin/$TARGET-qt6-qmake" ;;
    *) echo "usage: $0 qt6|qt5 [mxe dir]" >&2; exit 1 ;;
esac
[ -x "$QMAKE" ] || { echo "qmake not found: $QMAKE (build Qt in MXE first)" >&2; exit 1; }

export PATH="$MXE/usr/bin:$PATH"
BUILD="$DEPLOY/build/win-$QTVER"
mkdir -p "$BUILD" "$DEPLOY/dist"
cd "$BUILD"
"$QMAKE" "$SRC/FastECU.pro" CONFIG+=release CONFIG-=debug_and_release
make -j"${JOBS:-6}"

EXE="$(find "$BUILD" -maxdepth 2 -name FastECU.exe -newer "$SRC/FastECU.pro" | head -1)"
[ -n "$EXE" ] || EXE="$(find "$BUILD" -maxdepth 2 -name FastECU.exe | head -1)"
"$TARGET-strip" "$EXE"
OUT="$DEPLOY/dist/FastECU-$VERSION-win32-$QTVER.exe"
cp "$EXE" "$OUT"
echo "Built: $OUT"
# show DLL imports - should be Windows system DLLs only
"$TARGET-objdump" -p "$OUT" | grep 'DLL Name' | sort -u
