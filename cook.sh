#!/bin/bash

# TODO: Install Tiled exporter as well

echo "Cooking 16x16 mappings..."
for f in assets/levels/**/map16.json; do
    echo "Generating 16x16 mappings for ${f}..."
    ./tools/framepacker.py --tilemap "${f}" "`dirname ${f}`/MAP16.MAP"
done

echo "Cooking 16x16 collision..."
for f in assets/levels/**/tiles16.tsx; do
    echo "Generating collision for ${f}..."
    tiled --export-tileset "${f}" "`dirname ${f}`/collision16.json"
    ./tools/cookcollision.py "`dirname ${f}`/collision16.json" "`dirname ${f}`/MAP16.COL"
    rm "`dirname ${f}`/collision16.json"
done

echo "Cooking 128x128 mappings..."
for f in assets/levels/**/tilemap128.tmx; do
    echo "Generating 128x128 mappings for ${f}..."
    tiled --export-map "${f}" "${f%%.tmx}.csv"
    tmxrasterizer "${f}" "`dirname ${f}`/128.png"
    ./tools/chunkgen.py "${f%%.tmx}.csv" "`dirname ${f}`/MAP128.MAP"
    rm "${f%%.tmx}.csv"
done

echo "Cooking level maps..."
for f in assets/levels/**/Z*.tmx; do
    echo "Generating PSX LVL for ${f}..."
    tiled --export-map "${f}" "${f%%.tmx}.psxlvl"
    ./tools/cooklvl.py  "${f%%.tmx}.psxlvl" "${f%%.tmx}.LVL"
    rm "${f%%.tmx}.psxlvl"
done
