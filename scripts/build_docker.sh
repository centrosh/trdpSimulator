#!/usr/bin/env bash
set -euo pipefail

# Build the TRDP Simulator runtime Docker image.
# Usage: scripts/build_docker.sh [image-tag]

IMAGE_TAG="${1:-trdp-simulator:latest}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

cd "${PROJECT_ROOT}"

docker build \
  --file docker/Dockerfile \
  --tag "${IMAGE_TAG}" \
  .
