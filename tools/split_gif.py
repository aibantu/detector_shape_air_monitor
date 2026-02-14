#!/usr/bin/env python3
"""
split_gif.py

Usage:
  python tools\split_gif.py <input.gif> [--out data/res/frames] [--size 100]

This script extracts frames from an animated GIF and writes each frame as a
raw RGB565 little-endian file named frame000.raw, frame001.raw, ... into the
output directory. The output size will be scaled to `size x size` (default 100).

Requires: Pillow (PIL)
  pip install Pillow

After generating frames, put the `data/` folder in the project root and use
PlatformIO's Upload File System Image to write the frames into SPIFFS.
"""
import os
import sys
import argparse
from PIL import Image, ImageSequence

def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def main():
    p = argparse.ArgumentParser()
    p.add_argument('gif')
    p.add_argument('--out', default='data/res/frames')
    p.add_argument('--size', type=int, default=100)
    args = p.parse_args()

    if not os.path.exists(args.gif):
        print('Input GIF not found:', args.gif)
        sys.exit(2)

    os.makedirs(args.out, exist_ok=True)

    im = Image.open(args.gif)
    frame_idx = 0
    for frame in ImageSequence.Iterator(im):
        # Convert to RGBA then composite over white to flatten transparency
        frame = frame.convert('RGBA')
        bg = Image.new('RGBA', frame.size, (0,0,0,255))
        bg.paste(frame, (0,0), frame)
        frame = bg.convert('RGB')

        if args.size and (frame.width != args.size or frame.height != args.size):
            frame = frame.resize((args.size, args.size), Image.BILINEAR)

        out_path = os.path.join(args.out, f'frame{frame_idx:03d}.raw')
        with open(out_path, 'wb') as f:
            for y in range(args.size):
                for x in range(args.size):
                    r, g, b = frame.getpixel((x, y))
                    val = rgb_to_rgb565(r, g, b)
                    # write little-endian 16-bit
                    f.write(bytes((val & 0xFF, (val >> 8) & 0xFF)))
        print('Wrote', out_path)
        frame_idx += 1

    print('Total frames:', frame_idx)

if __name__ == '__main__':
    main()
