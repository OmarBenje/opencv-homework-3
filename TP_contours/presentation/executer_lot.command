#!/bin/bash
set -euo pipefail
TASK_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$TASK_ROOT"
if [ ! -x ./build/contours ]; then bash build.sh; fi
DEMO_OUTPUT="results/presentation_live_$(date +%Y%m%d_%H%M%S)"
./build/contours --batch --list data/test.txt --detector sobel --sigma 1 --kernel 3 --threshold 160 --save-images --output "$DEMO_OUTPUT/sobel"
./build/contours --batch --list data/test.txt --detector laplace --sigma 2 --kernel 3 --threshold 20 --save-images --output "$DEMO_OUTPUT/laplace"
./build/contours --batch --list data/test.txt --detector canny --sigma 2 --kernel 3 --low 40 --high 120 --save-images --output "$DEMO_OUTPUT/canny"
printf '\nRésultats de la démonstration : %s\n' "$DEMO_OUTPUT"
