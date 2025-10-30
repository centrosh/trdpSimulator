#!/usr/bin/env bash
set -euo pipefail

# Build the Python wheel and source distribution for the TRDP Simulator.
# Usage: scripts/build_python_package.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"

python3 -m pip install --upgrade pip build
python3 -m build
