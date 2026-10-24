#!/usr/bin/env python3
"""Regle sur 10 images, fige les parametres, puis evalue sur les 40 images."""
import csv
import itertools
import json
from pathlib import Path
import platform
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "build/contours"
RESULTS = ROOT / "results"

def rows(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))

def run(parameters, output, subset=None, save=False):
    command = [str(BINARY), "--batch", "--output", str(output)]
    for key, value in parameters.items():
        command += ["--" + key, str(value)]
    if subset:
        command += ["--list", str(subset)]
    if save:
        command += ["--save-images"]
    subprocess.run(command, cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
    return rows(output / "metrics.csv")

def summarize(items):
    counts = {key: sum(int(r[key]) for r in items) for key in ("detected", "reference", "correct", "fp", "fn")}
    denominator = counts["correct"] + counts["fp"] + counts["fn"]
    return {"n": len(items), **counts,
            "P_macro": statistics.mean(float(r["P"]) for r in items),
            "TFP_macro": statistics.mean(float(r["TFP"]) for r in items),
            "TFN_macro": statistics.mean(float(r["TFN"]) for r in items),
            "P_micro": counts["correct"] / denominator if denominator else 1,
            "detector_ms_mean": statistics.mean(float(r["detector_ms"]) for r in items)}

def main():
    start = time.time()
    RESULTS.mkdir(exist_ok=True)
    tuning_names = (ROOT / "data/tuning.txt").read_text().splitlines()
    test_names = (ROOT / "data/test.txt").read_text().splitlines()
    if set(tuning_names) & set(test_names):
        raise ValueError("Reglage et test doivent etre disjoints")
    search = []
    best = {}
    for detector in ("sobel", "laplace", "canny"):
        if detector == "canny":
            grid = [dict(detector=detector, sigma=s, kernel=3, low=low, high=low * ratio)
                    for s, low, ratio in itertools.product((0, 1, 2), (20, 40, 60, 80, 120, 160), (2, 3))]
        else:
            thresholds = (10, 20, 40, 80, 120, 160) if detector == "sobel" else (5, 10, 20, 40, 80, 120)
            grid = [dict(detector=detector, sigma=s, kernel=3, threshold=t)
                    for s, t in itertools.product((0, 1, 2), thresholds)]
        candidates = []
        for index, parameters in enumerate(grid):
            output = RESULTS / "search" / f"{detector}_{index:02d}"
            observed = run(parameters, output, ROOT / "data/tuning.txt")
            score = summarize([r for r in observed if r["radius"] == "1"])["P_macro"]
            record = {"parameters": parameters, "P_tuning": score, "csv": str(output.relative_to(ROOT) / "metrics.csv")}
            search.append(record)
            candidates.append(record)
        # Egalites departagees par l'ordre de grille, sans consulter le test.
        selected = max(candidates, key=lambda record: record["P_tuning"])
        best[detector] = selected["parameters"]
        print(f"{detector} : {best[detector]} ; P reglage={selected['P_tuning']:.4f}", flush=True)
    (RESULTS / "selected_parameters.json").write_text(json.dumps(best, indent=2) + "\n")
    (RESULTS / "search.json").write_text(json.dumps(search, indent=2) + "\n")
    all_rows = []
    summary = {}
    for detector, parameters in best.items():
        observed = run(parameters, RESULTS / detector, save=True)
        for row in observed:
            row["split"] = "tuning" if row["image"] in tuning_names else "test"
        all_rows.extend(observed)
        summary[detector] = {}
        for split in ("tuning", "test"):
            for radius in (0, 1):
                summary[detector][f"{split}_r{radius}"] = summarize(
                    [r for r in observed if r["split"] == split and r["radius"] == str(radius)])
    with (RESULTS / "all_metrics.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(all_rows[0]))
        writer.writeheader(); writer.writerows(all_rows)
    summary["protocol"] = {"tuning_images": tuning_names, "test_images": test_names,
                            "configurations": len(search), "selection": "P macro, tolerance 3x3, tuning only",
                            "kernel": 3, "elapsed_seconds": round(time.time() - start, 2),
                            "platform": platform.platform(), "python": platform.python_version()}
    (RESULTS / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    for detector in best:
        print(detector, "test", summary[detector]["test_r1"], flush=True)

if __name__ == "__main__":
    main()
