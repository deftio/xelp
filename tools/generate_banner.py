#!/usr/bin/env python3
"""
generate_banner.py -- Render text as ASCII art banners.

Requires Python 3.7 or later. No external dependencies.

Renders text as ASCII art using built-in fonts and optionally exports
the result as a C #define macro for embedding in header files.

Usage:
    python3 tools/generate_banner.py "Hello World"
    python3 tools/generate_banner.py "xelp v2.1" --font small
    python3 tools/generate_banner.py -f artwork.txt
    python3 tools/generate_banner.py "My App" --c-macro APP_BANNER
    python3 tools/generate_banner.py "My App" --c-macro APP_BANNER -o banner.h
    python3 tools/generate_banner.py "test" --raw

Fonts:
    standard   5-row figlet-style font (default)
    small      compact 3-row font, good for constrained targets
"""

import argparse
import sys

if sys.version_info < (3, 7):
    sys.stderr.write("Error: generate_banner.py requires Python 3.7 or later.\n")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Built-in fonts
# ---------------------------------------------------------------------------

# 5-row font inspired by figlet "small". Each glyph is a list of 5 strings.
# All glyphs for a given character must have the same width.
FONT_STANDARD = {
    'A': ["  ___  ",
          " / _ \\ ",
          "/ ___ \\",
          "/_/  \\_\\",
          "        "],
    'B': [" ___  ",
          "| _ ) ",
          "| _ \\ ",
          "|___/ ",
          "      "],
    'C': ["  ___ ",
          " / __|",
          "| (__ ",
          " \\___|",
          "      "],
    'D': [" ___  ",
          "|   \\ ",
          "| |) |",
          "|___/ ",
          "      "],
    'E': [" ___ ",
          "| __|",
          "| _| ",
          "|___|",
          "     "],
    'F': [" ___ ",
          "| __|",
          "| _| ",
          "|_|  ",
          "     "],
    'G': ["  ___ ",
          " / __|",
          "| (_ |",
          " \\___|",
          "      "],
    'H': [" _  _ ",
          "| || |",
          "| __ |",
          "|_||_|",
          "      "],
    'I': [" ___ ",
          "|_ _|",
          " | | ",
          "|___|",
          "     "],
    'J': ["   _ ",
          "  | |",
          "  | |",
          " _/ |",
          "|__/ "],
    'K': [" _  __",
          "| |/ /",
          "| ' < ",
          "|_|\\_\\",
          "      "],
    'L': [" _    ",
          "| |   ",
          "| |__ ",
          "|____|",
          "      "],
    'M': [" __  __ ",
          "|  \\/  |",
          "| |\\/| |",
          "|_|  |_|",
          "        "],
    'N': [" _  _ ",
          "| \\| |",
          "| .` |",
          "|_|\\_|",
          "      "],
    'O': ["  ___  ",
          " / _ \\ ",
          "| (_) |",
          " \\___/ ",
          "       "],
    'P': [" ___  ",
          "|  _ \\",
          "| |_) |",
          "|  __/ ",
          "|_|    "],
    'Q': ["  ___  ",
          " / _ \\ ",
          "| (_) |",
          " \\__\\_\\",
          "       "],
    'R': [" ___  ",
          "| _ \\ ",
          "|   / ",
          "|_|\\_\\",
          "      "],
    'S': [" ___ ",
          "/ __|",
          "\\__ \\",
          "|___/",
          "     "],
    'T': [" _____ ",
          "|_   _|",
          "  | |  ",
          "  |_|  ",
          "       "],
    'U': [" _   _ ",
          "| | | |",
          "| |_| |",
          " \\___/ ",
          "       "],
    'V': ["__   __",
          "\\ \\ / /",
          " \\ V / ",
          "  \\_/  ",
          "       "],
    'W': ["__      __",
          "\\ \\    / /",
          " \\ \\/\\/ / ",
          "  \\_/\\_/  ",
          "          "],
    'X': ["__  __",
          "\\ \\/ /",
          " >  < ",
          "/_/\\_\\",
          "      "],
    'Y': ["__   __",
          "\\ \\ / /",
          " \\ V / ",
          "  |_|  ",
          "       "],
    'Z': [" ____",
          "|_  /",
          " / / ",
          "/___|",
          "     "],
    '0': [" ___  ",
          "/ _ \\ ",
          "| () |",
          "\\___/ ",
          "      "],
    '1': [" _ ",
          "/ |",
          "| |",
          "|_|",
          "   "],
    '2': [" ___ ",
          "|_  )",
          " / / ",
          "/___|",
          "     "],
    '3': [" ___ ",
          "|__ \\",
          " / / ",
          "/___|",
          "     "],
    '4': [" _ _  ",
          "| | | ",
          "|_  _|",
          "  |_| ",
          "      "],
    '5': [" ___ ",
          "| __|",
          "|__ \\",
          "|___/",
          "     "],
    '6': ["  __ ",
          " / / ",
          "/ _ \\",
          "\\___/",
          "     "],
    '7': [" ____ ",
          "|__  |",
          "  / / ",
          " /_/  ",
          "      "],
    '8': [" ___ ",
          "( _ )",
          "/ _ \\",
          "\\___/",
          "     "],
    '9': [" ___ ",
          "/ _ \\",
          "\\_, /",
          " /_/ ",
          "     "],
    ' ': ["   ",
          "   ",
          "   ",
          "   ",
          "   "],
    '.': ["  ",
          "  ",
          "  ",
          "_ ",
          "  "],
    ',': ["  ",
          "  ",
          "  ",
          " ,",
          "  "],
    '!': [" _ ",
          "| |",
          "|_|",
          "(_)",
          "   "],
    '-': ["     ",
          "     ",
          " ___ ",
          "|___|",
          "     "],
    '_': ["      ",
          "      ",
          "      ",
          " ___ ",
          "|___|"],
    ':': ["   ",
          " _ ",
          "|_|",
          "|_|",
          "   "],
    '/': ["    /",
          "   / ",
          "  /  ",
          " /   ",
          "/    "],
    '(': ["  /",
          " / ",
          "|  ",
          " \\ ",
          "  \\"],
    ')': ["\\  ",
          " \\ ",
          "  |",
          " / ",
          "/  "],
    '@': [" ____  ",
          "/ __ \\ ",
          "| /_\\ |",
          "\\____/ ",
          "       "],
    '#': ["  # #  ",
          " #####",
          "  # # ",
          "##### ",
          " # #  "],
    '+': ["      ",
          "  _   ",
          " |+|  ",
          "  |   ",
          "      "],
    '=': ["     ",
          " ___ ",
          "|___|",
          "|___|",
          "     "],
    '>': ["\\   ",
          " \\  ",
          "  > ",
          " /  ",
          "/   "],
    '<': ["   /",
          "  / ",
          " <  ",
          "  \\ ",
          "   \\"],
    'v': ["     ",
          "     ",
          "\\ / ",
          " V  ",
          "    "],
}

# Compact 3-row font for constrained targets
FONT_SMALL = {
    'A': [" _ ", "/_\\", "/ \\"],
    'B': [" _ ", "|_)", "|_)"],
    'C': [" _ ", "|  ", "|_ "],
    'D': [" _ ", "| \\", "|_/"],
    'E': [" _ ", "|_ ", "|_ "],
    'F': [" _ ", "|_ ", "|  "],
    'G': [" _ ", "| _", "|_|"],
    'H': ["   ", "|_|", "| |"],
    'I': ["   ", " | ", " | "],
    'J': ["   ", "  |", "\\_|"],
    'K': ["   ", "|/ ", "|\\ "],
    'L': ["   ", "|  ", "|_ "],
    'M': ["    ", "|\\/|", "|  |"],
    'N': ["   ", "|\\ ", "| \\"],
    'O': [" _ ", "| |", "|_|"],
    'P': [" _ ", "|_)", "|  "],
    'Q': [" _ ", "| |", "|_\\"],
    'R': [" _ ", "|_)", "| \\"],
    'S': [" _ ", "|_ ", " _|"],
    'T': ["___", " | ", " | "],
    'U': ["   ", "| |", "|_|"],
    'V': ["   ", "\\ /", " v "],
    'W': ["    ", "|  |", "|/\\|"],
    'X': ["   ", "\\_/", "/ \\"],
    'Y': ["   ", "\\_/", " | "],
    'Z': ["__ ", " / ", "/_ "],
    '0': [" _ ", "| |", "|_|"],
    '1': ["   ", " | ", " | "],
    '2': [" _ ", " _|", "|_ "],
    '3': ["__ ", " _|", " _|"],
    '4': ["   ", "|_|", "  |"],
    '5': [" _ ", "|_ ", " _|"],
    '6': [" _ ", "|_ ", "|_|"],
    '7': ["__ ", "  |", "  |"],
    '8': [" _ ", "|_|", "|_|"],
    '9': [" _ ", "|_|", " _|"],
    ' ': ["  ", "  ", "  "],
    '-': ["   ", "---", "   "],
    '.': ["  ", "  ", ". "],
    '!': ["  ", "| ", "! "],
    '_': ["   ", "   ", "___"],
    ':': ["  ", "o ", "o "],
    '/': ["  /", " / ", "/  "],
    '+': ["   ", "_|_", " | "],
    '=': ["   ", "---", "---"],
    ',': ["  ", "  ", ", "],
}

FONTS = {
    'standard': (FONT_STANDARD, 5),
    'small': (FONT_SMALL, 3),
}


# ---------------------------------------------------------------------------
# Rendering
# ---------------------------------------------------------------------------

def _normalize_glyph(glyph, num_rows):
    """Pad all rows of a glyph to the same width and ensure correct row count."""
    if not glyph:
        return [" "] * num_rows
    width = max(len(row) for row in glyph)
    padded = [row.ljust(width) for row in glyph]
    while len(padded) < num_rows:
        padded.append(" " * width)
    return padded[:num_rows]


def render_text(text, font_name='standard'):
    """Render text string as multi-line ASCII art using the named font."""
    font_dict, num_rows = FONTS[font_name]
    rows = [""] * num_rows
    space_glyph = _normalize_glyph(font_dict.get(' ', [" "] * num_rows), num_rows)
    for ch in text:
        lookup = ch.upper()
        raw = font_dict.get(lookup)
        if raw is None:
            glyph = space_glyph
        else:
            glyph = _normalize_glyph(raw, num_rows)
        for i in range(num_rows):
            rows[i] += glyph[i]
    return "\n".join(rows) + "\n"


def escape_for_c(text, pad_width=0):
    """Escape a multi-line string for use in a C string literal."""
    lines = text.rstrip('\n').split('\n')
    if pad_width > 0:
        lines = [line.ljust(pad_width) for line in lines]
    escaped_lines = []
    for line in lines:
        line = line.replace('\\', '\\\\')
        line = line.replace('"', '\\"')
        escaped_lines.append(line)
    return '\\n'.join(escaped_lines) + '\\n'


def format_c_macro(name, escaped_str, raw_lines):
    """Wrap escaped string in a C #define macro with a size comment."""
    row_count = len(raw_lines)
    max_width = max(len(l) for l in raw_lines) if raw_lines else 0
    byte_count = sum(len(l) for l in raw_lines) + row_count + 1  # +newlines +null
    out = "/*\n"
    out += " * {rows} rows x {cols} cols, {b} bytes (incl null terminator)\n".format(
        rows=row_count, cols=max_width, b=byte_count)
    out += " */\n"
    out += '#define {name}  "{s}"\n'.format(name=name, s=escaped_str)
    return out


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        prog='generate_banner',
        description='Render text as ASCII art banners, with optional C macro export.',
        epilog=(
            'examples:\n'
            '  %(prog)s "Hello World"                         display banner\n'
            '  %(prog)s "xelp" --font small                   compact 3-row font\n'
            '  %(prog)s -f artwork.txt                         display from file\n'
            '  %(prog)s "My App" --c-macro APP_BANNER          output C #define\n'
            '  %(prog)s "My App" --c-macro APP_BANNER -o hdr.h write to file\n'
            '  %(prog)s "test" --raw                           escaped string only\n'
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument('text', nargs='*', default=None,
                        help='text to render as a banner')
    parser.add_argument('-f', '--file',
                        help='read ASCII art from a text file instead of rendering')
    parser.add_argument('--font', choices=sorted(FONTS.keys()), default='standard',
                        help='font to use (default: standard)')
    parser.add_argument('--c-macro', metavar='NAME', default=None,
                        help='output a C #define macro with this name')
    parser.add_argument('--raw', action='store_true',
                        help='output only the C-escaped string (no #define, no display)')
    parser.add_argument('-w', '--width', type=int, default=0,
                        help='pad each line to WIDTH characters')
    parser.add_argument('-o', '--output', metavar='FILE',
                        help='write output to FILE instead of stdout')

    args = parser.parse_args()

    # Determine input
    if args.file:
        with open(args.file, 'r') as f:
            banner = f.read()
    elif args.text:
        banner = render_text(' '.join(args.text), font_name=args.font)
    else:
        # No input -- show the default xelp banner
        banner = (
            "          _       \n"
            "__  _____| |_ __  \n"
            "\\ \\/ / _ \\ | '_ \\ \n"
            " >  <  __/ | |_) |\n"
            "/_/\\_\\___|_| .__/ \n"
            "           |_|    \n"
        )

    raw_lines = banner.rstrip('\n').split('\n')

    # Determine output mode
    if args.c_macro:
        escaped = escape_for_c(banner, pad_width=args.width)
        output_text = format_c_macro(args.c_macro, escaped, raw_lines)
    elif args.raw:
        escaped = escape_for_c(banner, pad_width=args.width)
        output_text = '"' + escaped + '"\n'
    else:
        output_text = banner

    # Write
    if args.output:
        with open(args.output, 'w') as f:
            f.write(output_text)
        sys.stderr.write("Written to {}\n".format(args.output))
    else:
        sys.stdout.write(output_text)

    # If we produced C output, also show the visual preview on stderr
    if (args.c_macro or args.raw) and not args.output:
        sys.stderr.write("\nPreview:\n")
        for line in raw_lines:
            sys.stderr.write("  " + line + "\n")


if __name__ == '__main__':
    main()
