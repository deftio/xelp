#!/usr/bin/env python3
"""extract_size.py -- extract code (.text) size from various object formats.

Usage:
    python3 extract_size.py <objfile> [--map <mapfile>]

Prints a single integer (bytes) on stdout, or "unavail" if extraction fails.

Supported formats (tried in order):
  1. ELF / a.out / COFF  -- via GNU `size` command
  2. SDCC .rel (ASxxxx)   -- sum all code areas (HOME, _CODE, GSINIT, ...)
  3. cc65 object          -- via `od65 --dump-segments`
  4. SDCC .map file        -- parse Area table for code sections
  5. Intel HEX (.ihx/.hex) -- compute total data bytes from records
"""

import argparse
import os
import re
import shutil
import subprocess
import sys


def try_gnu_size(objfile):
    """Strategy 1: GNU size (ELF, a.out, COFF)."""
    size_cmd = shutil.which("size")
    if not size_cmd:
        return None
    try:
        result = subprocess.run(
            [size_cmd, objfile],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode != 0:
            return None
        lines = result.stdout.strip().splitlines()
        if len(lines) < 2:
            return None
        # Berkeley format: text  data  bss  dec  hex  filename
        fields = lines[1].split()
        if fields:
            val = int(fields[0])
            if val > 0:
                return val
    except (subprocess.TimeoutExpired, ValueError, IndexError):
        pass
    return None


# Code areas in SDCC .rel files that contain executable code
SDCC_CODE_AREAS = {"_CODE", "HOME", "GSINIT", "GSFINAL", "CSEG"}


def try_sdcc_rel(objfile):
    """Strategy 2: SDCC .rel (ASxxxx relocatable).

    Area definition lines look like:
        A _CODE size 01A3 flags 0 addr 0
        A HOME size 004C flags 0 addr 0
    We sum sizes of all known code areas.
    """
    area_re = re.compile(
        r"^A\s+(\S+)\s+size\s+([0-9A-Fa-f]+)\s+flags\s",
    )
    total = 0
    found = False
    try:
        with open(objfile, "r", errors="replace") as f:
            for line in f:
                m = area_re.match(line)
                if m:
                    name = m.group(1)
                    if name in SDCC_CODE_AREAS:
                        total += int(m.group(2), 16)
                        found = True
    except (OSError, UnicodeDecodeError):
        return None
    return total if found else None


def try_od65(objfile):
    """Strategy 3: cc65 object format via od65."""
    od65_cmd = shutil.which("od65")
    if not od65_cmd:
        return None
    try:
        result = subprocess.run(
            [od65_cmd, "--dump-segments", objfile],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode != 0:
            return None
        # Look for:  Segment "CODE"  ...  Size: NNN
        in_code = False
        for line in result.stdout.splitlines():
            if 'Segment "CODE"' in line:
                in_code = True
            elif in_code and "Size:" in line:
                m = re.search(r"Size:\s*(\d+)", line)
                if m:
                    val = int(m.group(1))
                    if val > 0:
                        return val
                in_code = False
    except (subprocess.TimeoutExpired, ValueError):
        pass
    return None


SDCC_MAP_CODE_AREAS = {"_CODE", "HOME", "GSINIT", "GSFINAL", "CSEG"}


def try_sdcc_map(mapfile):
    """Strategy 4: SDCC .map file -- parse Area table for code sections.

    Lines like:
        .           .     _CODE                            01A3        419 = 0.41 KB
    """
    if not mapfile or not os.path.isfile(mapfile):
        return None
    area_re = re.compile(
        r"\.\s+\.\s+(\S+)\s+([0-9A-Fa-f]+)\s+"
    )
    total = 0
    found = False
    try:
        with open(mapfile, "r", errors="replace") as f:
            for line in f:
                m = area_re.search(line)
                if m:
                    name = m.group(1)
                    if name in SDCC_MAP_CODE_AREAS:
                        hex_sz = m.group(2)
                        total += int(hex_sz, 16)
                        found = True
    except (OSError, UnicodeDecodeError):
        return None
    return total if found else None


def try_intel_hex(objfile):
    """Strategy 5: Intel HEX (.ihx/.hex) -- total data bytes."""
    ext = os.path.splitext(objfile)[1].lower()
    if ext not in (".ihx", ".hex"):
        return None
    total = 0
    found = False
    try:
        with open(objfile, "r", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line.startswith(":"):
                    continue
                if len(line) < 11:
                    continue
                byte_count = int(line[1:3], 16)
                record_type = int(line[7:9], 16)
                if record_type == 0x00:  # data record
                    total += byte_count
                    found = True
                elif record_type == 0x01:  # EOF
                    break
    except (OSError, ValueError):
        return None
    return total if found else None


def extract_size(objfile, mapfile=None):
    """Try all strategies in order, return size or None."""
    # Strategy 1: GNU size
    sz = try_gnu_size(objfile)
    if sz is not None:
        return sz

    # Strategy 2: SDCC .rel
    sz = try_sdcc_rel(objfile)
    if sz is not None:
        return sz

    # Strategy 3: cc65 od65
    sz = try_od65(objfile)
    if sz is not None:
        return sz

    # Strategy 4: SDCC .map
    sz = try_sdcc_map(mapfile)
    if sz is not None:
        return sz

    # Strategy 5: Intel HEX
    sz = try_intel_hex(objfile)
    if sz is not None:
        return sz

    return None


def main():
    parser = argparse.ArgumentParser(
        description="Extract code (.text) size from various object file formats."
    )
    parser.add_argument("objfile", help="Object file to analyze")
    parser.add_argument("--map", default=None, help="SDCC .map file (optional)")
    args = parser.parse_args()

    if not os.path.isfile(args.objfile):
        print("unavail")
        sys.exit(0)

    sz = extract_size(args.objfile, mapfile=args.map)
    if sz is not None and sz > 0:
        print(sz)
    else:
        print("unavail")


if __name__ == "__main__":
    main()
