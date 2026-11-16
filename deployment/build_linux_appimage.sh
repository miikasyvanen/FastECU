#!/bin/bash
# Build a portable FastECU Linux AppImage inside an Ubuntu container (podman).
# The binary and its bundled libraries come from the container instead of the host, so the
# AppImage also runs on older distros:
#   x86_64:  Ubuntu 22.04 (glibc 2.35) -> Ubuntu 22.04+, Debian 12+, RHEL 9+
#   aarch64: Ubuntu 24.04 (glibc 2.39) -> Ubuntu 24.04+, Debian 13+, Raspberry Pi OS Trixie
#
# Usage: deployment/build_linux_appimage.sh [x86_64|aarch64]   (default: this machine's arch)
#   Building for the machine's own arch runs natively. The other arch runs the container
#   under qemu user emulation (slow, but no cross toolchain needed), which must be installed:
#   Fedora: sudo dnf install qemu-user-static-aarch64 (or -x86_64 on an aarch64 machine)
#   Debian/Ubuntu: sudo apt install qemu-user-static binfmt-support
# Build files go to deployment/build/linux-<arch>/ and deployment/build/AppDir-<arch>/,
# linuxdeploy to deployment/tools/<arch>/, the AppImage to deployment/dist/.
# Env: QT_VERSION (default 6.9.3), JOBS (default nproc), REBUILD_IMAGE=1 to rebuild the container image
set -euo pipefail

ARCH="${1:-$(uname -m)}"
[ "$ARCH" = arm64 ] && ARCH=aarch64
# aarch64 needs Ubuntu 24.04: Qt's official arm64 binaries require glibc 2.38
case "$ARCH" in
    x86_64)  PLATFORM=linux/amd64; BASE=ubuntu:22.04 ;;
    aarch64) PLATFORM=linux/arm64; BASE=ubuntu:24.04 ;;
    *) echo "usage: $0 [x86_64|aarch64]" >&2; exit 1 ;;
esac

DEPLOY="$(cd "$(dirname "$0")" && pwd)"
SRC="$(dirname "$DEPLOY")"
VERSION="$(grep -oP 'software_version = "\K[^"]+' "$SRC/file_actions.h")"
QT_VERSION="${QT_VERSION:-6.9.3}"
IMAGE="fastecu-build-${BASE/:/}-qt$QT_VERSION-$ARCH"
TOOLS="$DEPLOY/tools/$ARCH"

if [ "$ARCH" != "$(uname -m)" ] && ! grep -qs '^enabled' "/proc/sys/fs/binfmt_misc/qemu-$ARCH"; then
    echo "Cannot run $ARCH programs on this $(uname -m) host: qemu user emulation is not set up." >&2
    echo "Fedora: sudo dnf install qemu-user-static-$ARCH   Debian/Ubuntu: sudo apt install qemu-user-static binfmt-support" >&2
    exit 1
fi

mkdir -p "$DEPLOY/build" "$DEPLOY/dist" "$TOOLS"
cd "$TOOLS"
[ -f linuxdeploy-$ARCH.AppImage ] || \
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage
[ -f linuxdeploy-plugin-qt-$ARCH.AppImage ] || \
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage
chmod +x linuxdeploy*.AppImage
# AppImages carry the magic "AI\x02" in ELF header bytes 8..10, which keeps qemu's binfmt rule
# from matching them (Exec format error under emulation). Zero it; the tools work the same.
for f in linuxdeploy*.AppImage; do
    printf '\0\0\0' | dd of="$f" bs=1 seek=8 count=3 conv=notrunc status=none
done

if [ "${REBUILD_IMAGE:-0}" = 1 ] || ! podman image exists "$IMAGE"; then
    podman build --platform "$PLATFORM" --build-arg BASE="$BASE" --build-arg QT_VERSION="$QT_VERSION" -t "$IMAGE" \
        -f "$DEPLOY/linux/Containerfile" "$DEPLOY/linux"
fi

# label=disable: let the container read/write the bind mount without relabeling it (SELinux)
podman run --rm --platform "$PLATFORM" --security-opt label=disable -v "$SRC":/src \
    -e JOBS="${JOBS:-$(nproc)}" -e VERSION="$VERSION" -e ARCH="$ARCH" \
    "$IMAGE" bash -euo pipefail -c '
        D=/src/deployment
        mkdir -p $D/build/linux-$ARCH && cd $D/build/linux-$ARCH
        qmake /src/FastECU.pro CONFIG+=release CONFIG-=debug_and_release
        make -j"$JOBS"

        cd $D/build
        rm -rf AppDir-$ARCH
        export QMAKE=/opt/Qt/current/bin/qmake
        # linuxdeploy bundles an old strip that fails on current distro libraries
        export NO_STRIP=1
        export OUTPUT=$D/dist/FastECU-$VERSION-linux-$ARCH.AppImage
        rm -f "$OUTPUT"
        $D/tools/$ARCH/linuxdeploy-$ARCH.AppImage --appdir AppDir-$ARCH \
            -e linux-$ARCH/FastECU -i $D/linux/fastecu.png -d $D/linux/fastecu.desktop \
            --plugin qt --output appimage
    '

echo "Built: $DEPLOY/dist/FastECU-$VERSION-linux-$ARCH.AppImage"
