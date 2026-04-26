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
    mkdir -p "$REPO_ROOT/build"
    docker run --rm --platform linux/amd64 \
        -v "$REPO_ROOT/src:/xelp/src:ro" \
        -v "$SCRIPT_DIR/compactbuilds-docker.sh:/xelp/tools/compactbuilds-docker.sh:ro" \
        -v "$SCRIPT_DIR/extract_size.py:/xelp/tools/extract_size.py:ro" \
        -v "$REPO_ROOT/build:/xelp/build" \
        "$IMAGE_NAME"
    if [ -f "$REPO_ROOT/build/sizes.csv" ]; then
        echo ""
        echo "Size data written to build/sizes.csv"
        echo "Run 'bash tools/update_sizes.sh' to update docs."
    fi
}

case "${1:-}" in
    --build)
        do_build
        ;;
    --run)
        do_run
        ;;
    *)
        # Only rebuild if image doesn't exist; use --build to force
        if ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
            do_build
        else
            echo "Image '$IMAGE_NAME' exists (use --build to rebuild)."
        fi
        do_run
        ;;
esac
