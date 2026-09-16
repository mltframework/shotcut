#! /usr/bin/env python3

"""
make-ico.py

Build a Windows .ico from four square PNG images that are already at
the target pixel sizes (icons/shotcut-logo-<n>.png).

Each size is stored as a 32-bit PNG in the ICO (Windows Vista+). No
palettes and no dithering.

LICENSE: This program is put into the public domain by James Stroud, 2008.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from shlex import quote

SIZE_RE = re.compile(r'-(\d+)\.png$', re.IGNORECASE)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_IMAGES = [
  os.path.join(ROOT, 'icons', 'shotcut-logo-16.png'),
  os.path.join(ROOT, 'icons', 'shotcut-logo-24.png'),
  os.path.join(ROOT, 'icons', 'shotcut-logo-32.png'),
  os.path.join(ROOT, 'icons', 'shotcut-logo-48.png'),
]
DEFAULT_OUTPUT = os.path.join(ROOT, 'packaging', 'windows', 'shotcut-logo-64.ico')


def err(msg):
  sys.stderr.write("%s\n" % msg)


def run(command):
  err(command)
  status = os.system(command)
  if status != 0:
    sys.exit('command failed: %s' % command)


def size_from_filename(path):
  m = SIZE_RE.search(os.path.basename(path))
  if not m:
    sys.exit('filename must look like shotcut-logo-<n>.png: %s' % path)
  return int(m.group(1))


def png_dimensions(path):
  out = subprocess.check_output(
    ['identify', '-format', '%w %h', path], text=True).strip()
  width, height = out.split()
  return int(width), int(height)


def parse_args():
  parser = argparse.ArgumentParser(
    description='Build a Windows .ico from four pre-sized square PNG images.')
  parser.add_argument(
    'images', nargs='*',
    help='four PNGs named *-N.png (default: icons/shotcut-logo-{16,24,32,48}.png)')
  parser.add_argument(
    '-o', '--output', default=DEFAULT_OUTPUT,
    help='output .ico path (default: %s)' % DEFAULT_OUTPUT)
  args = parser.parse_args()
  if not args.images:
    args.images = DEFAULT_IMAGES
  elif len(args.images) != 4:
    parser.error('exactly 4 input images are required')
  return args


def main():
  args = parse_args()
  pngs = {}
  for path in args.images:
    if not os.path.exists(path):
      sys.exit('The image file given (%s) does not exist.' % path)
    size = size_from_filename(path)
    width, height = png_dimensions(path)
    if width != height:
      sys.exit('%s is %dx%d; a square image is required' % (path, width, height))
    if width != size:
      sys.exit('%s is %dx%d but the filename says %d' % (path, width, height, size))
    if size in pngs:
      sys.exit('duplicate size %d: %s and %s' % (size, pngs[size], path))
    pngs[size] = path

  sizes = sorted(pngs, reverse=True)

  workdir = tempfile.mkdtemp(prefix='shotcut-ico-')
  try:
    pams = []
    for size in sizes:
      png_path = pngs[size]
      pam_name = os.path.join(
        workdir, os.path.splitext(os.path.basename(png_path))[0] + '.pam')
      run("convert %s %s" % (quote(png_path), quote(pam_name)))
      pams.append(pam_name)

    # PNG-compress every size (default threshold is 128, which would BMP-encode
    # these). -truetransparent avoids XOR-mask inversion on the alpha.
    command = "cat %s | pamtowinicon -pngthreshold=1 -truetransparent > %s" % (
      " ".join(quote(p) for p in pams), quote(args.output))
    run(command)
  finally:
    shutil.rmtree(workdir, ignore_errors=True)

  err("Wrote %s" % args.output)


if __name__ == "__main__":
  main()
