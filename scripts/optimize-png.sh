#!/bin/sh
# Script to optimize and compress png impage files to reduce size.
# All operations are lossless
# Requires optipng and advpng
# In Ubuntu: "sudo apt-get install advancecomp optipng"

files=`find icons/ -name '*.PNG' -o -name '*.png'`

optipng -o 7 $files
advpng -z -4 -i 25 $files
# Sometimes 7z is actually smaller than zopfli
advpng -z -3 -i 25 $files

