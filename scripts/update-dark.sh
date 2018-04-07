#!/bin/sh
convert "$1" -negate -brightness-contrast -11.5 $(echo "$1" | sed 's/light/dark/')

