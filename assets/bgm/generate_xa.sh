#!/bin/bash
for f in *.flac; do
    psxavenc -f 37800 -t xa -b 4 -c 2 -F 1 -C 0 "$f" "${f%%.flac}.xa";
done

echo "Interleaving..."

for f in *.txt; do
    xainterleave 1 "$f" "${f%%.txt}.XA";
done

