#!/usr/bin/env python3
"""Controles independants des donnees, CSV et exports ; echoue au premier ecart."""
import csv
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]

def main():
    manifest = json.loads((ROOT / "data/provenance.json").read_text())
    assert len(manifest["files"]) == 80
    for entry in manifest["files"]:
        path = ROOT / entry["path"]
        assert hashlib.sha256(path.read_bytes()).hexdigest() == entry["sha256"]
        with Image.open(path) as image:
            assert image.size == (512, 512) and image.mode == "L"
            if "_gt_binary" in path.name:
                assert {value for value, count in enumerate(image.histogram()) if count} == {0, 255}
                assert image.histogram()[0] < image.histogram()[255], "Contours noirs attendus"
    rows = list(csv.DictReader((ROOT / "results/all_metrics.csv").open()))
    assert len(rows) == 240
    assert len({(r["image"], r["detector"], r["radius"]) for r in rows}) == 240
    for r in rows:
        d, ref, correct, fp, fn = [int(r[k]) for k in ("detected", "reference", "correct", "fp", "fn")]
        assert 0 <= correct <= min(d, ref)
        assert fp == d - correct and fn == ref - correct
        denominator = correct + fp + fn
        expected = [correct / denominator, fp / denominator, fn / denominator] if denominator else [1, 0, 0]
        assert all(abs(float(r[k]) - v) < 1e-9 for k, v in zip(("P", "TFP", "TFN"), expected))
        path = ROOT / "results" / r["detector"] / (r["image"] + "_" + r["detector"] + "_edges.png")
        with Image.open(path) as image:
            assert image.size == (512, 512)
            histogram = image.histogram()
            assert sum(histogram[1:255]) == 0 and histogram[255] == d
        with Image.open(ROOT / "data/images/gt" / (r["image"] + "_gt_binary.pgm")) as image:
            assert image.histogram()[0] == ref
    for r in rows:
        if r["radius"] == "1":
            exact = next(e for e in rows if e["image"] == r["image"] and e["detector"] == r["detector"] and e["radius"] == "0")
            assert int(r["correct"]) >= int(exact["correct"])
    search = json.loads((ROOT / "results/search.json").read_text())
    selected = json.loads((ROOT / "results/selected_parameters.json").read_text())
    summary = json.loads((ROOT / "results/summary.json").read_text())
    tuning = set((ROOT / "data/tuning.txt").read_text().splitlines())
    test = set((ROOT / "data/test.txt").read_text().splitlines())
    assert len(tuning) == 10 and len(test) == 30 and not tuning & test
    for configuration in search:
        rr = list(csv.DictReader((ROOT / configuration["csv"]).open()))
        assert len(rr) == 20 and {r["image"] for r in rr} == tuning
        assert abs(sum(float(r["P"]) for r in rr if r["radius"] == "1") / 10 - configuration["P_tuning"]) < 1e-10
    for detector, parameters in selected.items():
        candidates = [r for r in search if r["parameters"]["detector"] == detector]
        assert parameters == max(candidates, key=lambda r: r["P_tuning"])["parameters"]
        for split, names in [("test", test), ("tuning", tuning)]:
            for radius in ["0", "1"]:
                rr = [r for r in rows if r["detector"] == detector and r["radius"] == radius and r["image"] in names]
                assert abs(sum(float(r["P"]) for r in rr) / len(rr) - summary[detector][f"{split}_r{radius}"]["P_macro"]) < 1e-10
    # Erreurs de saisie et references manquantes : aucune evaluation silencieuse.
    invalid = [["--sigma", "nan"], ["--low", "120", "--high", "40"], ["--kernel", "4"],
               ["--tolerance", "2"], ["--sigma", "1garbage"]]
    for args in invalid:
        result = subprocess.run([str(ROOT / "build/contours"), "--batch", *args], cwd=ROOT, capture_output=True)
        assert result.returncode != 0 and b"Erreur" in result.stderr
    with tempfile.TemporaryDirectory() as directory:
        result = subprocess.run([str(ROOT / "build/contours"), "--batch", "--references", directory], cwd=ROOT, capture_output=True)
        assert result.returncode != 0 and b"Reference manquante" in result.stderr
    proof = "OK : 80 fichiers sources, 240 mesures, 120 cartes binaires, 72 configurations de reglage, selections et erreurs CLI.\n"
    (ROOT / "results/validation.txt").write_text(proof)
    print(proof, end="")

if __name__ == "__main__":
    main()
