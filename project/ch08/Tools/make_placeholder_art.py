#!/usr/bin/env python3
# GridSnake - course project for "Learn Unreal Engine 5".
#
# Generates the placeholder pixel art and sound effects used by the course, so the
# project needs no downloads. Python 3.8+ standard library only.
#
#   python Tools/make_placeholder_art.py
#
# Writes RawAssets/Textures/*.png and RawAssets/Audio/*.wav next to the .uproject.
# RawAssets/ is outside Content/, so nothing is imported until you drag the files
# into the Content Browser (see the chapter 4 and chapter 17 ASSETS.md files).

import math
import os
import random
import struct
import wave
import zlib

TILE = 16
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir, "RawAssets")

# ---------------------------------------------------------------- images


class Canvas:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.pixels = [(0, 0, 0, 0)] * (width * height)

    def set(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y * self.width + x] = color

    def rect(self, x0, y0, w, h, color):
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w):
                self.set(x, y, color)

    def disc(self, cx, cy, r, color):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= r * r:
                    self.set(x, y, color)

    def diamond(self, cx, cy, r, color):
        for y in range(cy - r, cy + r + 1):
            span = r - abs(y - cy)
            for x in range(cx - span, cx + span + 1):
                self.set(x, y, color)

    def save(self, path):
        raw = b"".join(
            b"\x00" + b"".join(bytes(self.pixels[y * self.width + x]) for x in range(self.width))
            for y in range(self.height)
        )

        def chunk(tag, data):
            body = tag + data
            return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

        header = struct.pack(">IIBBBBB", self.width, self.height, 8, 6, 0, 0, 0)  # 8-bit RGBA
        with open(path, "wb") as f:
            f.write(b"\x89PNG\r\n\x1a\n")
            f.write(chunk(b"IHDR", header))
            f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
            f.write(chunk(b"IEND", b""))


def rgb(hex_value, alpha=255):
    return ((hex_value >> 16) & 255, (hex_value >> 8) & 255, hex_value & 255, alpha)


FLOOR = rgb(0x2B2D42)
FLOOR_DARK = rgb(0x23253A)
MOSS = rgb(0x3F6B45)
RUG = rgb(0x7A3E48)
RUG_EDGE = rgb(0xC9A227)
STONE = rgb(0x8D99AE)
STONE_DARK = rgb(0x5C677D)
MORTAR = rgb(0x3D4357)
CRATE = rgb(0x9C6B3C)
CRATE_DARK = rgb(0x6B4423)
SNAKE = rgb(0x6ABE30)
SNAKE_DARK = rgb(0x37946E)
SNAKE_LIGHT = rgb(0x99E550)
EYE = rgb(0xFFFFFF)
PUPIL = rgb(0x111111)
TONGUE = rgb(0xD95763)
GEM = rgb(0x5FCDE4)
GEM_DARK = rgb(0x306082)
GEM_LIGHT = rgb(0xCBF1F5)
DOOR = rgb(0x6B4423)
BARS = rgb(0x9BADB7)
PORTAL = rgb(0x8E44AD)
PORTAL_LIGHT = rgb(0xD7A6F0)


def floor_tile(c, ox, oy, rng, accent=None):
    c.rect(ox, oy, TILE, TILE, FLOOR)
    for _ in range(10):
        c.set(ox + rng.randrange(TILE), oy + rng.randrange(TILE), FLOOR_DARK)
    if accent:
        for _ in range(18):
            c.set(ox + rng.randrange(TILE), oy + rng.randrange(TILE), accent)


def wall_tile(c, ox, oy, base, dark):
    c.rect(ox, oy, TILE, TILE, MORTAR)
    for row in range(4):
        shift = 0 if row % 2 == 0 else 4
        for col in range(-1, 3):
            x = ox + col * 8 + shift
            y = oy + row * 4
            for yy in range(y, y + 3):
                for xx in range(max(x, ox), min(x + 7, ox + TILE)):
                    c.set(xx, yy, base)
            for xx in range(max(x, ox), min(x + 7, ox + TILE)):
                c.set(xx, y + 2, dark)


def make_tiles():
    # 4 x 2 tiles. Row 0 = walkable (indices 0-3), row 1 = solid (indices 4-7).
    rng = random.Random(7)
    c = Canvas(TILE * 4, TILE * 2)
    floor_tile(c, 0, 0, rng)
    floor_tile(c, 16, 0, rng, FLOOR_DARK)
    floor_tile(c, 32, 0, rng, MOSS)
    c.rect(48, 0, TILE, TILE, RUG_EDGE)
    c.rect(49, 1, TILE - 2, TILE - 2, RUG)
    wall_tile(c, 0, 16, STONE, STONE_DARK)
    wall_tile(c, 16, 16, STONE_DARK, MORTAR)
    wall_tile(c, 32, 16, STONE, STONE_DARK)
    for _ in range(25):
        c.set(32 + rng.randrange(TILE), 16 + rng.randrange(TILE), MOSS)
    c.rect(48, 16, TILE, TILE, CRATE_DARK)
    c.rect(50, 18, TILE - 4, TILE - 4, CRATE)
    for i in range(TILE - 4):
        c.set(50 + i, 18 + i, CRATE_DARK)
    return c


def snake_head(c, ox, mouth=0, tongue=0, dead=False, fade=0):
    # Drawn facing +X (right); the game rotates it for the other directions.
    body = SNAKE if not dead else rgb(0x8A9A5B, 255 - fade)
    dark = SNAKE_DARK if not dead else rgb(0x5A6A3B, 255 - fade)
    c.disc(ox + 7, 8, 6.5, dark)
    c.disc(ox + 7, 8, 5.5, body)
    c.rect(ox + 0, 3, 6, 10, body)
    if mouth:
        for y in range(8 - mouth, 8 + mouth + 1):
            for x in range(ox + 13 - mouth * 2, ox + 16):
                c.set(x, y, (0, 0, 0, 0))
    if tongue:
        c.rect(ox + 13, 8, tongue, 1, TONGUE)
        c.set(ox + 12 + tongue, 7, TONGUE)
        c.set(ox + 12 + tongue, 9, TONGUE)
    for ey in (4, 11):
        if dead:
            c.set(ox + 8, ey - 1, PUPIL)
            c.set(ox + 10, ey + 1, PUPIL)
            c.set(ox + 9, ey, PUPIL)
            c.set(ox + 8, ey + 1, PUPIL)
            c.set(ox + 10, ey - 1, PUPIL)
        else:
            c.rect(ox + 8, ey - 1, 3, 3, EYE)
            c.set(ox + 10, ey, PUPIL)


def strip(frames):
    return Canvas(TILE * frames, TILE)


def make_head_idle():
    c = strip(2)
    snake_head(c, 0)
    snake_head(c, 16, tongue=3)
    return c


def make_head_move():
    c = strip(2)
    snake_head(c, 0, tongue=2)
    snake_head(c, 16)
    return c


def make_head_eat():
    c = strip(4)
    for i, mouth in enumerate((1, 3, 2, 0)):
        snake_head(c, i * 16, mouth=mouth)
    return c


def make_head_dead():
    c = strip(3)
    for i, fade in enumerate((0, 60, 120)):
        snake_head(c, i * 16, dead=True, fade=fade)
    return c


def make_body():
    c = strip(1)
    c.disc(8, 8, 7, SNAKE_DARK)
    c.disc(8, 8, 6, SNAKE)
    for x, y in ((5, 5), (9, 6), (6, 10), (10, 10)):
        c.rect(x, y, 2, 2, SNAKE_LIGHT)
    return c


def make_gem_idle():
    c = strip(6)
    for i in range(6):
        ox = i * 16
        c.diamond(ox + 8, 8, 6, GEM_DARK)
        c.diamond(ox + 8, 8, 5, GEM)
        c.diamond(ox + 7, 6, 1, GEM_LIGHT)
        sx, sy = [(3, 2), (12, 3), (13, 12), (2, 12), (8, 1), (8, 14)][i]
        c.set(ox + sx, sy, GEM_LIGHT)
        c.set(ox + sx - 1, sy, GEM_LIGHT)
        c.set(ox + sx + 1, sy, GEM_LIGHT)
        c.set(ox + sx, sy - 1, GEM_LIGHT)
        c.set(ox + sx, sy + 1, GEM_LIGHT)
    return c


def make_gem_pop():
    c = strip(5)
    for i in range(5):
        ox = i * 16
        r = 2 + i * 1.5
        for k in range(8):
            a = k * math.pi / 4
            x = ox + 8 + math.cos(a) * r
            y = 8 + math.sin(a) * r
            c.rect(int(x), int(y), 2 if i < 3 else 1, 2 if i < 3 else 1, GEM_LIGHT if i % 2 else GEM)
    return c


def exit_frame(c, ox, bars_open, portal):
    c.rect(ox, 0, TILE, TILE, STONE_DARK)
    c.rect(ox + 2, 2, 12, 14, DOOR if portal == 0 else PORTAL)
    if portal:
        c.disc(ox + 8, 9, 2 + portal, PORTAL_LIGHT)
        c.disc(ox + 8, 9, 1 + portal // 2, PORTAL)
    for bx in (3, 6, 9, 12):
        top = 2 + bars_open
        if top < 16:
            c.rect(ox + bx, 2, 1, 14 - bars_open, BARS)


def make_exit_closed():
    c = strip(1)
    exit_frame(c, 0, 0, 0)
    return c


def make_exit_opening():
    c = strip(4)
    for i in range(4):
        exit_frame(c, i * 16, 3 + i * 4, 0)
    return c


def make_exit_open():
    c = strip(4)
    for i in range(4):
        exit_frame(c, i * 16, 14, 1 + (i % 2) + (1 if i == 2 else 0))
    return c


TEXTURES = {
    "T_Tiles": make_tiles,
    "T_SnakeHead_Idle": make_head_idle,
    "T_SnakeHead_Move": make_head_move,
    "T_SnakeHead_Eat": make_head_eat,
    "T_SnakeHead_Dead": make_head_dead,
    "T_SnakeBody": make_body,
    "T_Gem_Idle": make_gem_idle,
    "T_Gem_Pop": make_gem_pop,
    "T_Exit_Closed": make_exit_closed,
    "T_Exit_Opening": make_exit_opening,
    "T_Exit_Open": make_exit_open,
}

# ---------------------------------------------------------------- sounds

RATE = 44100


def tone(freq, seconds, volume=0.4, shape="square", slide=0.0):
    samples = []
    phase = 0.0
    count = int(RATE * seconds)
    for i in range(count):
        t = i / count
        f = freq * (1.0 + slide * t)
        phase += f / RATE
        if shape == "square":
            v = 1.0 if (phase % 1.0) < 0.5 else -1.0
        elif shape == "triangle":
            v = 4.0 * abs((phase % 1.0) - 0.5) - 1.0
        else:
            v = math.sin(2.0 * math.pi * phase)
        envelope = min(1.0, i / 200.0) * (1.0 - t) ** 1.5
        samples.append(v * volume * envelope)
    return samples


def noise(seconds, volume=0.4):
    rng = random.Random(3)
    count = int(RATE * seconds)
    return [rng.uniform(-1, 1) * volume * (1.0 - i / count) ** 2 for i in range(count)]


def mix(*parts):
    out = [0.0] * max(len(p) for p in parts)
    for p in parts:
        for i, v in enumerate(p):
            out[i] += v
    return out


def sequence(notes, note_seconds, **kwargs):
    out = []
    for n in notes:
        out += tone(440.0 * 2 ** ((n - 69) / 12.0), note_seconds, **kwargs) if n else [0.0] * int(RATE * note_seconds)
    return out


def make_music():
    # Eight seconds at 120 BPM (16 beats) so it loops cleanly.
    beat = 0.5
    bass = sequence([45, 45, 48, 48, 43, 43, 40, 40] * 2, beat, volume=0.18, shape="triangle")
    arp_notes = []
    for root in (69, 72, 67, 64):
        arp_notes += [root, root + 4, root + 7, root + 12, root + 7, root + 4, root, root + 4] * 2
    arp = sequence(arp_notes[:64], beat / 4, volume=0.07)
    return mix(bass, arp)


SOUNDS = {
    "SW_Eat": lambda: tone(660, 0.12, slide=1.0),
    "SW_Die": lambda: mix(tone(330, 0.6, slide=-0.7), noise(0.6, 0.25)),
    "SW_ExitOpen": lambda: sequence([72, 76, 79, 84], 0.09),
    "SW_LevelComplete": lambda: sequence([72, 76, 79, 84, 0, 79, 84], 0.12, shape="triangle", volume=0.5),
    "SW_UIClick": lambda: tone(1200, 0.04, volume=0.25),
    "SW_Music": make_music,
}


def save_wav(path, samples):
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s)) * 32767)) for s in samples))


def main():
    texture_dir = os.path.join(ROOT, "Textures")
    audio_dir = os.path.join(ROOT, "Audio")
    os.makedirs(texture_dir, exist_ok=True)
    os.makedirs(audio_dir, exist_ok=True)

    for name, build in TEXTURES.items():
        canvas = build()
        canvas.save(os.path.join(texture_dir, name + ".png"))
        print("wrote Textures/%s.png (%dx%d)" % (name, canvas.width, canvas.height))

    for name, build in SOUNDS.items():
        samples = build()
        save_wav(os.path.join(audio_dir, name + ".wav"), samples)
        print("wrote Audio/%s.wav (%.2fs)" % (name, len(samples) / RATE))


if __name__ == "__main__":
    main()
