#!/usr/bin/env bash
set -euo pipefail

# Publish the Docker image to a registry.
# Usage: scripts/publish_docker.sh <image-tag> <registry>

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <image-tag> <registry>" >&2
  exit 1
fi

IMAGE_TAG="$1"
REGISTRY="$2"

FULL_TAG="${REGISTRY}/${IMAGE_TAG}"

docker tag "${IMAGE_TAG}" "${FULL_TAG}"
docker push "${FULL_TAG}"
