#!/bin/bash
# Build the FastECU Flatpak from the local source tree, export a single-file bundle and
# create the release zip.
# Usage: deployment/build_flatpak.sh [--install]
#   --install  also install the local build for the current user (flatpak run fi.fastecu.FastECU)
# Outputs in deployment/dist/:
#   FastECU-<ver>-linux-<arch>.flatpak    bundle built from the local tree
#   FastECU-<ver>-linux-flatpak.zip       manifest that fetches tag <ver> from GitHub and builds it
# Build files go to deployment/build/flatpak*/.
# <ver> is software_version in file_actions.h; push the matching git tag before publishing the zip.
set -euo pipefail

DEPLOY="$(cd "$(dirname "$0")" && pwd)"
SRC="$(dirname "$DEPLOY")"
VERSION="$(grep -oP 'software_version = "\K[^"]+' "$SRC/file_actions.h")"
ARCH="$(flatpak --default-arch)"
APP_ID=fi.fastecu.FastECU
MANIFEST="$DEPLOY/flatpak/$APP_ID.yml"
B="$DEPLOY/build"
DIST="$DEPLOY/dist"

INSTALL=()
[ "${1:-}" = "--install" ] && INSTALL=(--install)

grep -q '^    sources:$' "$MANIFEST" || { echo "no '    sources:' block in $MANIFEST" >&2; exit 1; }
mkdir -p "$B/flatpak-manifest" "$DIST"

# Local manifest: the sources block (last in the file) replaced with the local source tree
LOCAL="$B/flatpak-manifest/$APP_ID.yml"
sed '/^    sources:$/,$d' "$MANIFEST" > "$LOCAL"
cat >> "$LOCAL" <<EOF
    sources:
      - type: dir
        path: $SRC
        skip:
          - .git
          - build
          - deployment/build
          - deployment/dist
          - deployment/tools
EOF

flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

flatpak-builder --user --install-deps-from=flathub --force-clean \
    --state-dir="$B/flatpak-state" --repo="$B/flatpak-repo" "${INSTALL[@]}" \
    "$B/flatpak-build" "$LOCAL"

BUNDLE="$DIST/FastECU-$VERSION-linux-$ARCH.flatpak"
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    "$B/flatpak-repo" "$BUNDLE" "$APP_ID"
echo "Built: $BUNDLE"

# Release zip: the GitHub manifest with the tag set to this version
ZIPDIR="$B/flatpak-zip"
rm -rf "$ZIPDIR" && mkdir -p "$ZIPDIR"
sed "s/^\(        tag: \).*/\1$VERSION/" "$MANIFEST" > "$ZIPDIR/$APP_ID.yml"
grep -q "^        tag: $VERSION$" "$ZIPDIR/$APP_ID.yml" || { echo "tag line not found in $MANIFEST" >&2; exit 1; }
ZIP="$DIST/FastECU-$VERSION-linux-flatpak.zip"
rm -f "$ZIP"
(cd "$ZIPDIR" && zip -q "$ZIP" "$APP_ID.yml")
echo "Built: $ZIP"
if ! git -C "$SRC" ls-remote --exit-code --tags origin "refs/tags/$VERSION" >/dev/null 2>&1; then
    echo "NOTE: tag $VERSION is not on GitHub yet - push it before publishing the zip"
fi
