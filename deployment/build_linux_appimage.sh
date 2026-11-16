#!/bin/bash
# Build a portable FastECU x86_64 Linux AppImage inside an Ubuntu 22.04 container (podman).
# The binary and its bundled libraries come from Ubuntu 22.04 (glibc 2.35) instead of the host,
# so the AppImage also runs on older distros (Ubuntu 22.04+, Debian 12+, RHEL 9+).
#
# Usage: deployment/build_linux_appimage.sh
# Build files go to deployment/build/linux-ubuntu22.04/ and deployment/build/AppDir/,
# the AppImage to deployment/dist/.
# Env: QT_VERSION (default 6.9.3), JOBS (default 6), REBUILD_IMAGE=1 to rebuild the container image
set -euo pipefail

DEPLOY="$(cd "$(dirname "$0")" && pwd)"
SRC="$(dirname "$DEPLOY")"
VERSION="$(grep -oP 'software_version = "\K[^"]+' "$SRC/file_actions.h")"
QT_VERSION="${QT_VERSION:-6.9.3}"
IMAGE="fastecu-build-ubuntu22.04-qt$QT_VERSION"

mkdir -p "$DEPLOY/build" "$DEPLOY/dist" "$DEPLOY/tools"
cd "$DEPLOY/tools"
[ -f linuxdeploy-x86_64.AppImage ] || \
    wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
[ -f linuxdeploy-plugin-qt-x86_64.AppImage ] || \
    wget https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x linuxdeploy*.AppImage

if [ "${REBUILD_IMAGE:-0}" = 1 ] || ! podman image exists "$IMAGE"; then
    podman build --build-arg QT_VERSION="$QT_VERSION" -t "$IMAGE" -f "$DEPLOY/linux/Containerfile" "$DEPLOY/linux"
fi

# label=disable: let the container read/write the bind mount without relabeling it (SELinux)
podman run --rm --security-opt label=disable -v "$SRC":/src \
    -e JOBS="${JOBS:-6}" -e QT_VERSION="$QT_VERSION" -e VERSION="$VERSION" \
    "$IMAGE" bash -euo pipefail -c '
        D=/src/deployment
        mkdir -p $D/build/linux-ubuntu22.04 && cd $D/build/linux-ubuntu22.04
        qmake /src/FastECU.pro CONFIG+=release CONFIG-=debug_and_release
        make -j"$JOBS"

        cd $D/build
        rm -rf AppDir
        export QMAKE=/opt/Qt/$QT_VERSION/gcc_64/bin/qmake
        # linuxdeploy bundles an old strip that fails on current distro libraries
        export NO_STRIP=1
        export OUTPUT=$D/dist/FastECU-$VERSION-linux-x86_64.AppImage
        rm -f "$OUTPUT"
        $D/tools/linuxdeploy-x86_64.AppImage --appdir AppDir \
            -e linux-ubuntu22.04/FastECU -i $D/linux/fastecu.png -d $D/linux/fastecu.desktop \
            --plugin qt --output appimage
    '

echo "Built: $DEPLOY/dist/FastECU-$VERSION-linux-x86_64.AppImage"
