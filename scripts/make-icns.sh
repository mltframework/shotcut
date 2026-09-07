#!/bin/sh
# Build packaging/macos/shotcut.icns from the logo images.
# Uses icons/shotcut-logo-64.png for sizes <= 64 and the no-text 1440 master
# for everything larger. PNG sources need sips, iconutil, and swift (macOS).
# SVG sources are rasterized first with rsvg-convert, magick, or inkscape.
# Each size is composited on a #323232 rounded rectangle.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SOURCE_64="${SOURCE_64:-$ROOT/icons/shotcut-logo-64.png}"
SOURCE_LARGE="${SOURCE_LARGE:-$ROOT/icons/shotcut-logo-large.png}"
OUTPUT="${OUTPUT:-$ROOT/packaging/macos/shotcut.icns}"
# Corner radius as a fraction of the image's shorter side (macOS icon template ~0.18).
ICON_CORNER_RATIO="${ICON_CORNER_RATIO:-0.18}"
# Padding between the mark and the tile edge, as a fraction of the shorter side.
# 0.08 = 8% on each edge at every resolution (about 82px at 1024).
ICON_PAD_RATIO="${ICON_PAD_RATIO:-0.08}"
ICON_BG_COLOR="${ICON_BG_COLOR:-#323232}"

usage() {
    cat <<EOF
Usage: $(basename "$0")

Build $OUTPUT from:
  <= 64 px  $SOURCE_64
  >  64 px  $SOURCE_LARGE

Sources may be PNG or SVG. SVG requires one of: rsvg-convert, magick, inkscape.
A #323232 rounded-rectangle background is composited behind the mark.
Override paths with SOURCE_64, SOURCE_LARGE, and OUTPUT.
Override the tile with ICON_BG_COLOR, ICON_CORNER_RATIO, and ICON_PAD_RATIO.
EOF
}

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    usage
    exit 0
fi

for cmd in sips iconutil swift; do
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

is_svg() {
    case "$1" in
        *.svg | *.SVG) return 0 ;;
        *) return 1 ;;
    esac
}

svg_to_png() {
    src=$1
    size=$2
    dest=$3
    if command -v rsvg-convert >/dev/null 2>&1; then
        rsvg-convert -w "$size" -h "$size" -b none -o "$dest" "$src"
    elif command -v magick >/dev/null 2>&1; then
        magick -background none "$src" -resize "${size}x${size}" "$dest"
    elif command -v convert >/dev/null 2>&1; then
        convert -background none "$src" -resize "${size}x${size}" "$dest"
    elif command -v inkscape >/dev/null 2>&1; then
        inkscape -w "$size" -h "$size" --export-background-opacity=0 \
            --export-filename="$dest" "$src"
    else
        echo "$(basename "$0"): SVG input needs rsvg-convert, magick, or inkscape" >&2
        echo "  macOS: brew install librsvg   or   sudo port install librsvg" >&2
        exit 1
    fi
}

# Composite src onto a rounded-rectangle background of ICON_BG_COLOR.
# ICON_PAD_RATIO is a fraction of the shorter side, applied on every edge.
add_rounded_background() {
    src=$1
    dest=$2
    swift - "$src" "$dest" "$ICON_BG_COLOR" "$ICON_CORNER_RATIO" "$ICON_PAD_RATIO" <<'SWIFT'
import AppKit
import Foundation

let args = CommandLine.arguments
guard args.count >= 6 else { exit(1) }
let srcPath = args[1]
let destPath = args[2]
let colorStr = args[3]
let ratio = CGFloat(Double(args[4]) ?? 0.18)
let padRatio = CGFloat(Double(args[5]) ?? 0.08)

func parseHexColor(_ s: String) -> NSColor {
    var hex = s.trimmingCharacters(in: .whitespacesAndNewlines)
    if hex.hasPrefix("#") { hex.removeFirst() }
    guard hex.count == 6, let v = UInt32(hex, radix: 16) else {
        fputs("invalid ICON_BG_COLOR \(s)\n", stderr)
        exit(1)
    }
    let r = CGFloat((v >> 16) & 0xff) / 255
    let g = CGFloat((v >> 8) & 0xff) / 255
    let b = CGFloat(v & 0xff) / 255
    return NSColor(srgbRed: r, green: g, blue: b, alpha: 1)
}

guard let src = NSImage(contentsOf: URL(fileURLWithPath: srcPath)) else {
    fputs("failed to load \(srcPath)\n", stderr)
    exit(1)
}
let pxW = src.representations.first.map { $0.pixelsWide } ?? Int(src.size.width)
let pxH = src.representations.first.map { $0.pixelsHigh } ?? Int(src.size.height)
guard let rep = NSBitmapImageRep(
    bitmapDataPlanes: nil, pixelsWide: pxW, pixelsHigh: pxH,
    bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
    colorSpaceName: .calibratedRGB, bytesPerRow: 0, bitsPerPixel: 0)
else { exit(1) }
rep.size = NSSize(width: pxW, height: pxH)

guard let nsCtx = NSGraphicsContext(bitmapImageRep: rep) else { exit(1) }
NSGraphicsContext.saveGraphicsState()
NSGraphicsContext.current = nsCtx
let ctx = nsCtx.cgContext
let rect = CGRect(x: 0, y: 0, width: pxW, height: pxH)
let radius = min(rect.width, rect.height) * ratio
let path = CGPath(roundedRect: rect, cornerWidth: radius, cornerHeight: radius, transform: nil)
ctx.addPath(path)
ctx.clip()
ctx.setFillColor(parseHexColor(colorStr).cgColor)
ctx.fill(rect)
if let cg = src.cgImage(forProposedRect: nil, context: nil, hints: nil) {
    let pad = min(rect.width, rect.height) * padRatio
    ctx.draw(cg, in: rect.insetBy(dx: pad, dy: pad))
}
NSGraphicsContext.restoreGraphicsState()

guard let png = rep.representation(using: .png, properties: [:]) else { exit(1) }
do {
    try png.write(to: URL(fileURLWithPath: destPath))
} catch {
    fputs("\(error)\n", stderr)
    exit(1)
}
SWIFT
}

TMPDIR_ICNS=$(mktemp -d "${TMPDIR:-/tmp}/shotcut.XXXXXX")
ICONSET="$TMPDIR_ICNS/shotcut.iconset"
mkdir "$ICONSET"
trap 'rm -rf "$TMPDIR_ICNS"' EXIT

RASTER_64=$SOURCE_64
RASTER_LARGE=$SOURCE_LARGE
if is_svg "$SOURCE_64"; then
    RASTER_64="$TMPDIR_ICNS/source-64.png"
    echo "  rasterize $(basename "$SOURCE_64") -> 64x64"
    svg_to_png "$SOURCE_64" 64 "$RASTER_64"
fi
if is_svg "$SOURCE_LARGE"; then
    RASTER_LARGE="$TMPDIR_ICNS/source-large.png"
    echo "  rasterize $(basename "$SOURCE_LARGE") -> 1024x1024"
    svg_to_png "$SOURCE_LARGE" 1024 "$RASTER_LARGE"
fi

BG_64="$TMPDIR_ICNS/bg-64.png"
BG_LARGE="$TMPDIR_ICNS/bg-large.png"
echo "  background $ICON_BG_COLOR radius=${ICON_CORNER_RATIO} pad=${ICON_PAD_RATIO} on $(basename "$SOURCE_64")"
add_rounded_background "$RASTER_64" "$BG_64"
echo "  background $ICON_BG_COLOR radius=${ICON_CORNER_RATIO} pad=${ICON_PAD_RATIO} on $(basename "$SOURCE_LARGE")"
add_rounded_background "$RASTER_LARGE" "$BG_LARGE"
RASTER_64=$BG_64
RASTER_LARGE=$BG_LARGE

source_for_size() {
    if [ "$1" -le 64 ]; then
        printf '%s\n' "$RASTER_64"
    else
        printf '%s\n' "$RASTER_LARGE"
    fi
}

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
    if [ "$size" -le 64 ]; then orig=$SOURCE_64; else orig=$SOURCE_LARGE; fi
    echo "  $name (${size}x${size}) <- $(basename "$orig")"
    sips -z "$size" "$size" "$src" --out "$ICONSET/$name" >/dev/null
done

iconutil -c icns "$ICONSET" -o "$OUTPUT"
echo "Wrote $OUTPUT"
