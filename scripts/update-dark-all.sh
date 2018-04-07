for f in light/32x32/*.png; do convert "$f" -negate -brightness-contrast -11.5 $(echo "$f" | sed 's/light/dark/'); done
