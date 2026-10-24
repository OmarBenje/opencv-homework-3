#!/usr/bin/env python3
"""Archive de remise portable, sans binaires locaux ni fichiers temporaires."""
from pathlib import Path
import zipfile

ROOT = Path(__file__).resolve().parents[1]
target = ROOT.parent / "TP_contours.zip"
with zipfile.ZipFile(target, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
    for path in sorted(ROOT.rglob("*")):
        relative = path.relative_to(ROOT)
        if not path.is_file() or any(part in {"build", "tmp", ".venv", "__pycache__", ".git"} for part in relative.parts):
            continue
        if path.name in {".DS_Store", "contours.tar.gz"}:
            continue
        archive.write(path, Path("TP_contours") / relative)
with zipfile.ZipFile(target) as archive:
    assert archive.testzip() is None
    for name in ["src/main.cpp", "src/contours.cpp", "report/rapport.pdf", "video/README.md", "results/all_metrics.csv"]:
        assert "TP_contours/" + name in archive.namelist()
    assert len([n for n in archive.namelist() if n.startswith("TP_contours/data/images/") and n.endswith(".pgm")]) == 80
    print(f"Archive vérifiée : {len(archive.namelist())} fichiers, {target.stat().st_size / 1_000_000:.1f} Mo, {target}")
