#!/bin/bash
# crossbuild.sh -- build and run the xelp cross-compilation Docker container
#
# Usage:
#   bash tools/crossbuild.sh              # build image + run report
#   bash tools/crossbuild.sh --build      # only rebuild the Docker image
#   bash tools/crossbuild.sh --run        # only run (image must exist)
#
# Run from the repository root directory.

set -e

IMAGE_NAME="xelp-crossbuild"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

do_build() {
    echo "Building Docker image '$IMAGE_NAME' (linux/amd64)..."
    docker build --platform linux/amd64 \
        -t "$IMAGE_NAME" -f "$SCRIPT_DIR/Dockerfile.crossbuild" "$REPO_ROOT"
    echo "Image built successfully."
}

do_run() {
    echo "Running cross-compilation report..."
    docker run --rm --platform linux/amd64 \
        -v "$SCRIPT_DIR/compactbuilds-docker.sh:/xelp/tools/compactbuilds-docker.sh:ro" \
        "$IMAGE_NAME"
}

case "${1:-}" in
    --build)
        do_build
        ;;
    --run)
        do_run
        ;;
    *)
        do_build
        do_run
        ;;
esac
