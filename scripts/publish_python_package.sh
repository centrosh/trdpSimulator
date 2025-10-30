#!/usr/bin/env bash
set -euo pipefail

# Publish the Python distributions to PyPI or a compatible index.
# Usage: scripts/publish_python_package.sh [repository-url]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

REPOSITORY_URL="${1:-https://upload.pypi.org/legacy/}"

cd "${PROJECT_ROOT}"

if [[ ! -d "dist" ]]; then
  echo "Distribution artifacts not found. Run scripts/build_python_package.sh first." >&2
  exit 1
fi

python3 -m pip install --upgrade twine
python3 -m twine upload --repository-url "${REPOSITORY_URL}" dist/*
