#! /usr/bin/env python

"""
make-ico.py

LICENSE: This program is put into the public domain by James Stroud, 2008.
"""

import sys
import os
import base64

win16map = """\
UDYKMTYgMQoyNTUKAAAAgAAAAP//AP8AwMDAAICA/wD//wAAgICAAACAgACA//////8AAIAAgIAA
AAD/
"""

def err(msg):
  sys.stderr.write("%s\n" % msg)

def usage(err_msg=None):
  if err_msg is not None:
    err("\n** ERROR **: %s\n" % err_msg)
  progname = os.path.basename(sys.argv[0])
  err("usage: python %s imagefile\n" % progname)
  sys.exit()


def main():
  try:
    png_file = sys.argv[1]
  except IndexError:
    usage('No image file given.')

  if not os.path.exists(png_file):
    usage('The image file given (%s) does not exist.' % png_file)

  png_base, extension = os.path.basename(png_file).rsplit('.', 1)


  to_delete = []

  sizes = [48, 32, 24, 16]
  depths = [4, 8, 24]

  # these commands are redundant at this point
  # however, if this is generalized, this will need to be done
  # to ensure the final .ico file is created correctly
  sizes.sort(reverse=True)
  depths.sort()

  to_map = {}
  for size in sizes:
    name_args = (png_base, size, size)
    resized_base = "%s-%02dx%02d" % name_args
    resized_name = "%s.png" % resized_base
    resize_args = (png_file, size, size, resized_name)
    command = 'convert %s -resize %dx%d %s' % resize_args
    err(command)
    os.system(command)
    to_map[size] = resized_base
    to_delete.append(resized_name)

  ico_parts = []
  for depth in depths:
    for size in sizes:
      resized_base = to_map[size]
      resized_name = "%s.png" % resized_base
      redepthed_base = "%s-%02d" % (resized_base, depth)
      redepthed_name = "%s.pnm" % redepthed_base
      redepth_args = (depth, resized_name, redepthed_name)
      if depth >= 8:
        command = "convert -depth %d %s %s" % redepth_args
      else:
        command = "convert %s %s" % (resized_name, redepthed_name)
      err(command)
      os.system(command)
      to_delete.append(redepthed_name)
      map_base = "%s-%02d" % (resized_base, depth)
      map_name = "%s.pam" % map_base
      if depth >= 8:
        colors = 256
        map_args = (colors, redepthed_name, map_name)
        command = "pnmcolormap %d %s > %s" % map_args
        err(command)
        os.system(command)
      else:
        # for the < 8 bit images, we don't need to calculate the map
        open(map_name, 'wb').write(base64.decodestring(win16map))
      to_delete.append(map_name)
      remapped_base = map_base
      remapped_name = "%s.ppm" % remapped_base
      remap_args = (map_name, redepthed_name, remapped_name)
      command = "pnmremap -mapfile=%s -fs %s > %s" % remap_args
      err(command)
      os.system(command)
      to_delete.append(remapped_name)
      ico_parts.append(remapped_name)


  icon_names = " ".join(ico_parts)
  icon_name = "%s.ico" % png_base
  icon_args = (icon_names, icon_name)

  command = 'ppmtowinicon %s --output %s' % icon_args
  err(command)
  os.system(command)

  err("rm %s" % " ".join(to_delete))
  for p in to_delete:
    os.remove(p)

if __name__ == "__main__":
  main()
