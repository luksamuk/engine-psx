#!/bin/bash

for f in *.ogg; do
	ffmpeg -y -i "$f" -acodec pcm_s16le -ac 1 -ar 22050 "${f%%.ogg}.WAV";
	wav2vag "${f%%.ogg}.WAV" "${f%%.ogg}.VAG";
	rm "${f%%.ogg}.WAV";
done

