#!/bin/bash

for f in *.mp4; do
	ffmpeg -y -i "$f" \
		-vcodec rawvideo -pix_fmt bgr24 -vf scale=320:240,setsar=1:1,vflip -r 15 \
		-an \
		tmp.avi
	mencoder tmp.avi -ovc copy -o "${f%%.mp4}.AVI"
	rm tmp.avi
	ffmpeg -y -i "$f" -acodec pcm_s16le -ar 44100 "${f%%.mp4}_AUDIO.WAV"
done

echo "Make sure you now use MC32.EXE and:"
echo "- Generate an audioless .STR;"
echo "- Generate a .XA from the .WAV;"
echo "- Mix both audio and video into a single .STR."

