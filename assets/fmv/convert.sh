#!/bin/bash

for f in *.mp4; do
    psxavenc -t str2 -f 37800 -b 4 -c 2 -s 320x240 -r 15 -x 2 "$f" "${f%%.mp4}.STR"
done

