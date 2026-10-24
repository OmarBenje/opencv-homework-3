#!/usr/bin/env python3
"""Compile le rapport LaTeX et exporte le PDF et les sources autonomes."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "report/latex"

def main():
    (ROOT / "tmp").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="latex-", dir=ROOT / "tmp") as build:
        if shutil.which("tectonic"):
            subprocess.run(["tectonic", "--outdir", build, "main.tex"], cwd=PROJECT, check=True)
        elif shutil.which("latexmk"):
            subprocess.run(["latexmk", "-pdf", "-interaction=nonstopmode", "-halt-on-error",
                            "-outdir=" + build, "main.tex"], cwd=PROJECT, check=True)
        elif shutil.which("pdflatex"):
            command = ["pdflatex", "-interaction=nonstopmode", "-halt-on-error", "-output-directory", build, "main.tex"]
            subprocess.run(command, cwd=PROJECT, check=True)
            subprocess.run(command, cwd=PROJECT, check=True)
        else:
            raise SystemExit("Installer Tectonic ou TeX Live, ou importer report/latex dans Overleaf.")
        pdf = Path(build) / "main.pdf"
        if not pdf.is_file():
            raise RuntimeError("Compilation terminee sans fichier PDF.")
        shutil.copy2(pdf, ROOT / "report/rapport.pdf")
    archive = ROOT / "report/rapport-latex.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
        for path in sorted(PROJECT.rglob("*")):
            if path.is_file() and path.suffix in {".tex", ".pdf", ".png", ".md"}:
                bundle.write(path, path.relative_to(PROJECT))
    with zipfile.ZipFile(archive) as bundle:
        assert bundle.testzip() is None
        assert "main.tex" in bundle.namelist()
    print(ROOT / "report/rapport.pdf")
    print(archive)

if __name__ == "__main__":
    main()
