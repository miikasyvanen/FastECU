#!/bin/bash
# Cross-compile a single static FastECU.exe for Windows on Linux, using MXE.
#   qt6 -> Windows 10/11
#   qt5 -> Windows 7 (also runs on 10/11)
# Architecture defaults to 32-bit; pass 64 (or --64) for a 64-bit build.
# Usage: deployment/build_windows.sh [qt6|qt5] [32|64] [MXE dir, default ~/.mxe]
#   (the three tokens may be given in any order; anything unrecognised is the MXE dir)
# Build files go to deployment/build/win<arch>-<qt>/, the .exe to deployment/dist/.
set -euo pipefail

QTVER=qt6
ARCH=32
MXE="${MXE_DIR:-$HOME/.mxe}"

for arg in "$@"; do
    case "$arg" in
        qt5|qt6)              QTVER="$arg" ;;
        32|--32|win32|x86)    ARCH=32 ;;
        64|--64|win64|x86_64) ARCH=64 ;;
        *)                    MXE="$arg" ;;
    esac
done

case "$ARCH" in
    32) TARGET=i686-w64-mingw32.static ;;
    64) TARGET=x86_64-w64-mingw32.static ;;
    *)  echo "usage: $0 [qt6|qt5] [32|64] [mxe dir]" >&2; exit 1 ;;
esac

DEPLOY="$(cd "$(dirname "$0")" && pwd)"
SRC="$(dirname "$DEPLOY")"
VERSION="$(grep -oP 'software_version = "\K[^"]+' "$SRC/file_actions.h")"

case "$QTVER" in
    qt5) QMAKE="$MXE/usr/bin/$TARGET-qmake-qt5" ;;
    qt6) QMAKE="$MXE/usr/bin/$TARGET-qt6-qmake" ;;
    *) echo "usage: $0 [qt6|qt5] [32|64] [mxe dir]" >&2; exit 1 ;;
esac
[ -x "$QMAKE" ] || { echo "qmake not found: $QMAKE (build the $TARGET Qt in MXE first)" >&2; exit 1; }

export PATH="$MXE/usr/bin:$PATH"
BUILD="$DEPLOY/build/win$ARCH-$QTVER"
mkdir -p "$BUILD" "$DEPLOY/dist"
cd "$BUILD"
"$QMAKE" "$SRC/FastECU.pro" CONFIG+=release CONFIG-=debug_and_release
make -j"${JOBS:-6}"

EXE="$(find "$BUILD" -maxdepth 2 -name FastECU.exe -newer "$SRC/FastECU.pro" | head -1)"
[ -n "$EXE" ] || EXE="$(find "$BUILD" -maxdepth 2 -name FastECU.exe | head -1)"
"$TARGET-strip" "$EXE"
OUT="$DEPLOY/dist/FastECU-$VERSION-win$ARCH-$QTVER.exe"
cp "$EXE" "$OUT"
echo "Built: $OUT"
# show DLL imports - should be Windows system DLLs only
"$TARGET-objdump" -p "$OUT" | grep 'DLL Name' | sort -u
