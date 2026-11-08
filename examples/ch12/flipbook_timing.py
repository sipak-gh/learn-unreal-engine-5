#!/usr/bin/env python3
# Example for chapter 12 of "Learn Unreal Engine 5". Python 3.8+, standard library only.
#
# Prints the length of every GridSnake flipbook from the strips that
# Tools/make_placeholder_art.py generates, and the PlayRate that would make a
# looping flipbook play exactly one cycle per snake step.
#
#   python3 flipbook_timing.py <path to RawAssets/Textures> [step interval in s]

import os
import struct
import sys

TILE = 16  # every frame is 16x16 px, side by side in one row

# Frames Per Second, as set in chapter 4 (project/ch04/ASSETS.md).
FPS = {
    "SnakeHead_Idle": 4, "SnakeHead_Move": 8, "SnakeHead_Eat": 16, "SnakeHead_Dead": 8,
    "Gem_Idle": 8, "Gem_Pop": 16, "Exit_Opening": 10, "Exit_Open": 8,
}
ONE_SHOT = {"SnakeHead_Eat", "SnakeHead_Dead", "Gem_Pop", "Exit_Opening"}


def png_width(path):
    # A PNG starts with an 8-byte signature and the IHDR chunk header (8 bytes);
    # width and height follow as big-endian 32-bit integers.
    with open(path, "rb") as f:
        header = f.read(24)
    width, _height = struct.unpack(">II", header[16:24])
    return width


def main():
    folder = sys.argv[1]
    step = float(sys.argv[2]) if len(sys.argv) > 2 else 0.15
    print("%-20s %6s %4s %8s  %-4s  %s" % ("flipbook", "frames", "fps", "length", "mode", "PlayRate for 1 cycle/step"))
    for name, fps in FPS.items():
        frames = png_width(os.path.join(folder, "T_%s.png" % name)) // TILE
        length = frames / fps
        mode = "once" if name in ONE_SHOT else "loop"
        rate = "-" if mode == "once" else "%.2f" % (length / step)
        print("%-20s %6d %4d %7.3fs  %-4s  %s" % ("FB_" + name, frames, fps, length, mode, rate))


if __name__ == "__main__":
    main()
