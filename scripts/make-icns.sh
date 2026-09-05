#!/bin/sh
# Build packaging/macos/shotcut.icns from the logo PNGs.
# Uses icons/shotcut-logo-64.png for sizes <= 64 and the no-text 1440 master
# for everything larger. Requires sips and iconutil (macOS).

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SOURCE_64="${SOURCE_64:-$ROOT/icons/shotcut-logo-64.png}"
SOURCE_LARGE="${SOURCE_LARGE:-$ROOT/icons/shotcut-logo-1440.png}"
OUTPUT="${OUTPUT:-$ROOT/packaging/macos/shotcut.icns}"

usage() {
    cat <<EOF
Usage: $(basename "$0")

Build $OUTPUT from:
  <= 64 px  $SOURCE_64
  >  64 px  $SOURCE_LARGE

Override paths with SOURCE_64, SOURCE_LARGE, and OUTPUT.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

for cmd in sips iconutil; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        echo "$(basename "$0"): $cmd is required (macOS)" >&2
        exit 1
    fi
done

for src in "$SOURCE_64" "$SOURCE_LARGE"; do
    if [ ! -f "$src" ]; then
        echo "$(basename "$0"): missing source image: $src" >&2
        exit 1
    fi
done

source_for_size() {
    if [ "$1" -le 64 ]; then
        printf '%s\n' "$SOURCE_64"
    else
        printf '%s\n' "$SOURCE_LARGE"
    fi
}

TMPDIR_ICNS=$(mktemp -d "${TMPDIR:-/tmp}/shotcut.XXXXXX")
ICONSET="$TMPDIR_ICNS/shotcut.iconset"
mkdir "$ICONSET"
trap 'rm -rf "$TMPDIR_ICNS"' EXIT

# name size
# iconutil expects these exact filenames; size is the pixel dimension.
set -- \
    icon_16x16.png 16 \
    icon_16x16@2x.png 32 \
    icon_32x32.png 32 \
    icon_32x32@2x.png 64 \
    icon_128x128.png 128 \
    icon_128x128@2x.png 256 \
    icon_256x256.png 256 \
    icon_256x256@2x.png 512 \
    icon_512x512.png 512 \
    icon_512x512@2x.png 1024

while [ "$#" -ge 2 ]; do
    name=$1
    size=$2
    shift 2
    src=$(source_for_size "$size")
    echo "  $name (${size}x${size}) <- $(basename "$src")"
    sips -z "$size" "$size" "$src" --out "$ICONSET/$name" >/dev/null
done

iconutil -c icns "$ICONSET" -o "$OUTPUT"
echo "Wrote $OUTPUT"
