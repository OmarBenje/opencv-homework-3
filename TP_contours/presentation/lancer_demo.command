#!/bin/bash
set -euo pipefail
TASK_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$TASK_ROOT"
if [ ! -x ./build/contours ]; then bash build.sh; fi
DEMO_OUTPUT="results/presentation_live_$(date +%Y%m%d_%H%M%S)"
exec ./build/contours --gui --list data/test.txt --detector canny --sigma 2 --kernel 3 --low 40 --high 120 --tolerance 1 --output "$DEMO_OUTPUT"
