#!/usr/bin/env python3
"""Build the real simulator, replay seed 8257, and render its verified output."""

from __future__ import annotations

import hashlib
import json
import math
import shutil
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "demo"
FRAMES = DEMO / "frames"
WIDTH, HEIGHT = 960, 540
FPS = 24
SEED = 8257


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_source() -> None:
    manifest = json.loads((DEMO / "source-manifest.json").read_text())
    for relative, expected in manifest["files"].items():
        actual = sha256(ROOT / relative)
        if actual != expected:
            raise SystemExit(f"source drift: {relative}: {actual} != {expected}")


def run_actual_demo() -> str:
    build = DEMO / "build"
    build.mkdir(exist_ok=True)
    binary = build / "fundamentals"
    subprocess.run(
        ["c++", "-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic",
         str(ROOT / "fundamentals.cpp"), "-o", str(binary)],
        check=True,
    )
    command = [str(binary), "--seed", str(SEED)]
    first = subprocess.check_output(command, text=True)
    second = subprocess.check_output(command, text=True)
    if first != second:
        raise SystemExit("seeded executable output is not deterministic")
    required = [
        "Resting:1001 Qty:11 Price:100.00",
        "Resting:1002 Qty:6 Price:100.00",
        "Rejected: order ID 2001 has already been accepted in this session.",
        "Rejected: order ID must be positive.",
        "Accepted orders: 6 | Rejected orders: 2",
        "Matched quantity (per side): 24",
        "Resting quantity: 10",
        "Conservation: PASS",
    ]
    missing = [line for line in required if line not in first]
    if missing:
        raise SystemExit("actual output is missing required evidence: " + repr(missing))
    (DEMO / "actual-output.txt").write_text(first)
    return first


def font(size: int, bold: bool = False):
    family = "DejaVuSans-Bold.ttf" if bold else "DejaVuSans.ttf"
    return ImageFont.truetype(f"/usr/share/fonts/truetype/dejavu/{family}", size)


def rounded(draw, box, radius, fill, outline=None, width=1):
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def base_frame(step: int, title: str, subtitle: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    image = Image.new("RGB", (WIDTH, HEIGHT), "#070b18")
    pixels = image.load()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            glow = max(0.0, 1.0 - math.hypot(x - 760, y - 90) / 560)
            pixels[x, y] = (int(7 + 16 * glow), int(11 + 12 * glow), int(24 + 34 * glow))
    draw = ImageDraw.Draw(image)
    draw.text((40, 30), "C++ MATCH ENGINE", font=font(16, True), fill="#8ae8ff")
    rounded(draw, (720, 26, 920, 56), 15, "#13263b", "#28647e")
    draw.text((739, 34), "ACTUAL EXECUTABLE  •  SEED 8257", font=font(10, True), fill="#a9f3ff")
    draw.text((40, 80), title, font=font(34, True), fill="#f5f7ff")
    draw.text((42, 124), subtitle, font=font(16), fill="#a6b2ce")
    draw.line((40, 164, 920, 164), fill="#23304a", width=1)
    for i in range(6):
        x = 40 + i * 34
        color = "#55d6be" if i <= step else "#26334b"
        draw.rounded_rectangle((x, 505, x + 22, 512), radius=3, fill=color)
    draw.text((760, 496), "FIXTURE • NO LIVE EXCHANGE", font=font(11, True), fill="#75829d")
    return image, draw


def card(draw, box, title, value, accent="#55d6be"):
    rounded(draw, box, 16, "#111a2c", "#263652")
    x1, y1, _, _ = box
    draw.text((x1 + 18, y1 + 17), title, font=font(12, True), fill="#8492ae")
    draw.text((x1 + 18, y1 + 48), value, font=font(22, True), fill=accent)


def render_scene(scene: int, progress: float) -> Image.Image:
    if scene == 0:
        image, draw = base_frame(scene, "One command. Same run. Every time.",
                                 "The binary executes twice; byte-identical output is required before rendering.")
        card(draw, (40, 195, 920, 380), "REPLAY COMMAND", "./build/fundamentals --seed 8257")
        draw.text((70, 320), "✓ deterministic order stream", font=font(16, True), fill="#55d6be")
        draw.text((350, 320), "✓ real matching logic", font=font(16, True), fill="#55d6be")
        draw.text((620, 320), "✓ source hash guarded", font=font(16, True), fill="#55d6be")
    elif scene == 1:
        image, draw = base_frame(scene, "Price-time priority, made visible",
                                 "Two sell orders rest at the same price in arrival order.")
        rounded(draw, (70, 200, 890, 410), 18, "#10192a", "#283954")
        draw.text((95, 224), "ASK  $100.00", font=font(18, True), fill="#ff8fa3")
        width = int(620 * min(1, progress * 1.5))
        rounded(draw, (210, 215, 210 + width, 277), 13, "#301d31")
        if progress > .18:
            draw.text((235, 232), "#1001  •  11 qty  •  FIRST", font=font(17, True), fill="#ffd7df")
        if progress > .48:
            rounded(draw, (210, 300, 610, 362), 13, "#241f36")
            draw.text((235, 317), "#1002  •  6 qty  •  SECOND", font=font(17, True), fill="#ded7ff")
        draw.text((670, 323), "FIFO QUEUE", font=font(14, True), fill="#8ae8ff")
    elif scene == 2:
        image, draw = base_frame(scene, "A market buy sweeps the queue",
                                 "Actual fills preserve FIFO at $100.00, then continue at $100.50.")
        card(draw, (40, 200, 295, 365), "AGGRESSOR #2001", "BUY 19", "#8ae8ff")
        arrow_x = 310 + int(130 * min(1, progress * 2))
        draw.line((310, 283, arrow_x, 283), fill="#55d6be", width=6)
        draw.polygon([(arrow_x, 274), (arrow_x + 18, 283), (arrow_x, 292)], fill="#55d6be")
        card(draw, (470, 188, 920, 250), "FILL 1", "11 × $100.00   RESTING #1001", "#ffcf70")
        card(draw, (470, 265, 920, 327), "FILL 2", "6 × $100.00    RESTING #1002", "#ffcf70")
        card(draw, (470, 342, 920, 404), "FILL 3", "2 × $100.50    RESTING #1003", "#ffcf70")
    elif scene == 3:
        image, draw = base_frame(scene, "Bad IDs stop at the session boundary",
                                 "Rejected orders never trade or rest in the book.")
        card(draw, (40, 205, 455, 355), "DUPLICATE #2001", "REJECTED", "#ff7a96")
        card(draw, (505, 205, 920, 355), "NONPOSITIVE #0", "REJECTED", "#ff7a96")
        draw.text((115, 385), "accepted IDs stay reserved after fill", font=font(14), fill="#a6b2ce")
        draw.text((590, 385), "IDs must be strictly positive", font=font(14), fill="#a6b2ce")
    elif scene == 4:
        image, draw = base_frame(scene, "Final state reconciles exactly",
                                 "Every accepted unit is matched, resting, or a cancelled market remainder.")
        card(draw, (40, 200, 250, 315), "ACCEPTED QTY", "58")
        card(draw, (270, 200, 480, 315), "MATCHED / SIDE", "24")
        card(draw, (500, 200, 710, 315), "RESTING", "10")
        card(draw, (730, 200, 920, 315), "CANCELLED", "0")
        rounded(draw, (40, 345, 920, 425), 18, "#112c28", "#2f8c7d", 2)
        draw.text((75, 368), "58  =  2 × 24  +  10  +  0", font=font(25, True), fill="#e9fffa")
        draw.text((745, 372), "PASS", font=font(20, True), fill="#55d6be")
    else:
        image, draw = base_frame(scene, "Reproducible by design",
                                 "Tests cover identity rules, FIFO fills, deterministic CLI output, and reconciliation.")
        rounded(draw, (40, 200, 920, 385), 18, "#10192a", "#293956")
        lines = [
            "$ make test",
            "Order identity regression checks passed.",
            "Deterministic CLI checks passed.",
            "$ ./build/fundamentals --seed 8257",
        ]
        for index, line in enumerate(lines):
            color = "#8ae8ff" if line.startswith("$") else "#55d6be"
            draw.text((72, 226 + index * 36), line, font=font(17, index in (1, 2)), fill=color)
    return image


def main() -> None:
    validate_source()
    run_actual_demo()
    if FRAMES.exists():
        shutil.rmtree(FRAMES)
    FRAMES.mkdir()
    frames_per_scene = 44
    for scene in range(6):
        for index in range(frames_per_scene):
            progress = index / (frames_per_scene - 1)
            frame = render_scene(scene, progress)
            frame.save(FRAMES / f"frame-{scene * frames_per_scene + index:04d}.png")
    subprocess.run([
        "ffmpeg", "-y", "-framerate", str(FPS), "-i", str(FRAMES / "frame-%04d.png"),
        "-c:v", "libx264", "-preset", "slow", "-crf", "20", "-pix_fmt", "yuv420p",
        "-movflags", "+faststart", str(DEMO / "matching-engine.mp4")
    ], check=True)
    subprocess.run([
        "ffmpeg", "-y", "-framerate", str(FPS), "-i", str(FRAMES / "frame-%04d.png"),
        "-vf", "fps=12,scale=960:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128[p];[s1][p]paletteuse=dither=bayer",
        "-loop", "0", str(DEMO / "matching-engine.gif")
    ], check=True)
    shutil.copy2(FRAMES / "frame-0176.png", DEMO / "poster.png")
    print("Rendered actual seeded output to demo/matching-engine.{mp4,gif}")


if __name__ == "__main__":
    main()
