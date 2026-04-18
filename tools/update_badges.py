#!/usr/bin/env python3
"""
update_badges.py -- Update shields.io version badges in project files.

Reads the version from build/xelp_version.yaml and replaces all
badge/version-{ANY}-blue.svg patterns in the target files.

Usage:
    python3 tools/update_badges.py
    python3 tools/update_badges.py --dry-run
    python3 tools/update_badges.py --version-file path/to/version.yaml
    python3 tools/update_badges.py --files README.md pages/index.html
"""

import argparse
import re
import sys

DEFAULT_VERSION_FILE = "build/xelp_version.yaml"
DEFAULT_FILES = ["README.md", "pages/index.html"]

BADGE_PATTERN = re.compile(r'badge/version-[^-]+-blue\.svg')


def read_version(path):
    """Extract version string from a YAML file (line-based, no PyYAML)."""
    try:
        with open(path, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('version:'):
                    # version: "0.2.3"
                    match = re.search(r'"([^"]+)"', line)
                    if match:
                        return match.group(1)
    except FileNotFoundError:
        sys.stderr.write(f"Error: version file not found: {path}\n")
        sys.exit(1)

    sys.stderr.write(f"Error: no version string found in {path}\n")
    sys.exit(1)


def update_file(path, version, dry_run=False):
    """Replace badge version patterns in a file. Returns True if changed."""
    try:
        with open(path, 'r') as f:
            original = f.read()
    except FileNotFoundError:
        sys.stderr.write(f"Warning: file not found, skipping: {path}\n")
        return False

    replacement = f'badge/version-{version}-blue.svg'
    updated = BADGE_PATTERN.sub(replacement, original)

    if updated == original:
        sys.stderr.write(f"  {path}: already up to date\n")
        return False

    if dry_run:
        sys.stderr.write(f"  {path}: would update to {version}\n")
    else:
        with open(path, 'w') as f:
            f.write(updated)
        sys.stderr.write(f"  {path}: updated to {version}\n")

    return True


def main():
    parser = argparse.ArgumentParser(
        prog='update_badges',
        description='Update shields.io version badges in project files.',
    )
    parser.add_argument('--version-file', default=DEFAULT_VERSION_FILE,
                        help=f'path to version YAML (default: {DEFAULT_VERSION_FILE})')
    parser.add_argument('--files', nargs='+', default=DEFAULT_FILES,
                        metavar='FILE',
                        help='files to update (default: README.md pages/index.html)')
    parser.add_argument('--dry-run', action='store_true',
                        help='preview changes without writing')

    args = parser.parse_args()
    version = read_version(args.version_file)

    sys.stderr.write(f"Version: {version}\n")
    if args.dry_run:
        sys.stderr.write("Dry run -- no files will be modified.\n")

    changed = 0
    for path in args.files:
        if update_file(path, version, dry_run=args.dry_run):
            changed += 1

    if changed == 0:
        sys.stderr.write("All badges already up to date.\n")
    elif args.dry_run:
        sys.stderr.write(f"{changed} file(s) would be updated.\n")
    else:
        sys.stderr.write(f"{changed} file(s) updated.\n")


if __name__ == '__main__':
    main()
