#!/usr/bin/env python3
"""Telecharge et extrait uniquement les PGM reguliers du jeu de Groningen."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
URL = "https://www.cs.rug.nl/~imaging/databases/contour_database/contours.tar.gz"
PAGE = "https://www.cs.rug.nl/~imaging/databases/contour_database/contour_database.html"

def main():
    data = ROOT / "data"
    data.mkdir(exist_ok=True)
    archive = data / "contours.tar.gz"
    if not archive.exists():
        urllib.request.urlretrieve(URL, archive)
    with tarfile.open(archive, "r:gz") as tar:
        for member in tar.getmembers():
            relative = PurePosixPath(member.name)
            if not member.isfile() or relative.suffix != ".pgm":
                continue
            if relative.is_absolute() or ".." in relative.parts or relative.parts[0] != "images":
                raise ValueError(f"Chemin d'archive invalide : {member.name}")
            target = data.joinpath(*relative.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with tar.extractfile(member) as source:
                target.write_bytes(source.read())
    images = sorted((data / "images").glob("*.pgm"))
    if len(images) != 40:
        raise ValueError(f"40 images attendues, {len(images)} obtenues")
    files = []
    for image in images:
        reference = image.parent / "gt" / (image.stem + "_gt_binary.pgm")
        if not reference.is_file():
            raise ValueError(f"Reference manquante : {reference}")
        for path in (image, reference):
            files.append({"path": str(path.relative_to(ROOT)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    manifest = {"source": PAGE, "archive": URL, "archive_sha256": hashlib.sha256(archive.read_bytes()).hexdigest(),
                "citation": "C. Grigorescu, N. Petkov, M. A. Westenberg. Contour detection based on non-classical receptive field inhibition. IEEE TIP 12(7), 729-739, 2003.",
                "image_count": 40, "files": files}
    (data / "provenance.json").write_text(json.dumps(manifest, indent=2) + "\n")
    # Repartition fixee avant recherche : dix positions regulierement espacees.
    names = [p.stem for p in images]
    tuning = names[::4]
    test = [n for n in names if n not in tuning]
    (data / "tuning.txt").write_text("\n".join(tuning) + "\n")
    (data / "test.txt").write_text("\n".join(test) + "\n")
    print(f"40 paires disponibles ; {len(tuning)} reglage, {len(test)} evaluation.")

if __name__ == "__main__":
    main()
