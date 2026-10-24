#!/usr/bin/env python3
"""Figures construites uniquement a partir des mesures et sorties du C++."""
import csv
import json
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "report/figures"
OUT.mkdir(parents=True, exist_ok=True)
summary = json.loads((ROOT / "results/summary.json").read_text())
search = json.loads((ROOT / "results/search.json").read_text())
rows = list(csv.DictReader((ROOT / "results/all_metrics.csv").open()))
COLORS = ["#3679b5", "#de8843", "#169c86"]
DETECTORS = ["sobel", "laplace", "canny"]
plt.rcParams.update({"font.family": "DejaVu Sans", "axes.spines.top": False, "axes.spines.right": False,
                     "font.size": 11, "axes.labelcolor": "#23344a", "text.color": "#23344a"})

fig, ax = plt.subplots(figsize=(9.5, 3.2))
x = np.arange(3)
ax.bar(x - .18, [summary[d]["test_r0"]["P_macro"] for d in DETECTORS], .34, label="Position exacte", color="#aab9cc")
bars = ax.bar(x + .18, [summary[d]["test_r1"]["P_macro"] for d in DETECTORS], .34, label="Tolérance 3 × 3", color=COLORS)
ax.set_xticks(x, ["Sobel", "Laplace", "Canny"])
ax.set_ylabel("Performance moyenne P")
ax.set_ylim(0, .34)
ax.bar_label(bars, fmt="%.3f", padding=4)
ax.legend(frameon=False, loc="upper left")
ax.grid(axis="y", alpha=.15); ax.set_axisbelow(True)
fig.tight_layout(); fig.savefig(OUT / "performance.png", dpi=180); plt.close(fig)

fig, axes = plt.subplots(1, 3, figsize=(12, 3.3), sharey=True)
for ax, detector in zip(axes, DETECTORS):
    for sigma, color in zip((0, 1, 2), COLORS):
        selected = [r for r in search if r["parameters"]["detector"] == detector
                    and r["parameters"]["sigma"] == sigma
                    and (detector != "canny" or r["parameters"]["high"] == 3 * r["parameters"]["low"])]
        key = "low" if detector == "canny" else "threshold"
        selected.sort(key=lambda r: r["parameters"][key])
        ax.plot([r["parameters"][key] for r in selected], [r["P_tuning"] for r in selected],
                "o-", markersize=3, label=f"σ = {sigma}", color=color)
    ax.set_title(detector.capitalize())
    ax.set_xlabel("Seuil bas (haut = 3 × bas)" if detector == "canny" else "Seuil de réponse")
    ax.grid(alpha=.15)
axes[0].set_ylabel("P moyen sur les 10 images de réglage")
axes[-1].legend(frameon=False)
fig.tight_layout(); fig.savefig(OUT / "sensitivity.png", dpi=180); plt.close(fig)

try:
    font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 25)
except OSError:
    font = ImageFont.truetype("DejaVuSans.ttf", 25)
for name in ("gnu", "goat_5", "golfcart"):
    paths = [ROOT / "data/images" / f"{name}.pgm", ROOT / "data/images/gt" / f"{name}_gt_binary.pgm"]
    paths += [ROOT / "results" / d / f"{name}_{d}_edges.png" for d in DETECTORS]
    canvas = Image.new("RGB", (5 * 512, 570), "white")
    draw = ImageDraw.Draw(canvas)
    for index, (path, title) in enumerate(zip(paths, ["Original", "Référence", "Sobel", "Laplace", "Canny"])):
        image = Image.open(path).convert("RGB")
        # Inversion pour comparer visuellement les contours blancs des sorties C++.
        if index == 1:
            image = Image.fromarray(255 - np.asarray(image))
        canvas.paste(image, (index * 512, 58))
        draw.text((index * 512 + 16, 15), title, font=font, fill="#23344a")
    canvas.save(OUT / f"{name}_comparison.png")
    canvas = Image.new("RGB", (1536, 570), "white")
    draw = ImageDraw.Draw(canvas)
    for index, detector in enumerate(DETECTORS):
        row = next(r for r in rows if r["image"] == name and r["detector"] == detector and r["radius"] == "1")
        draw.text((index * 512 + 16, 15), f"{detector.capitalize()} · P = {float(row['P']):.3f}", font=font, fill="#23344a")
        canvas.paste(Image.open(ROOT / "results" / detector / f"{name}_{detector}_overlay.png"), (index * 512, 58))
    canvas.save(OUT / f"{name}_errors.png")
print("8 figures exportées dans report/figures.")
