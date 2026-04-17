/*
 * extract_version.c -- Read XELP_VERSION from xelp.h and emit YAML.
 *
 * This is a build-time tool, not part of the xelp library.  It
 * #includes the real xelp.h so the version is resolved by the C
 * preprocessor -- no regex, no string parsing.
 *
 * Usage:
 *   gcc tools/extract_version.c -Isrc -o build/extract_version
 *   build/extract_version                      # YAML to stdout
 *   build/extract_version build/version.yaml   # YAML to file
 *
 * The output is consumed by make_release.sh and CI workflows.
 */

#include <stdio.h>
#include "xelp.h"

int main(int argc, char *argv[])
{
    unsigned long ver = XELP_VERSION;
    int major = XELP_VER_MAJOR(ver);
    int minor = XELP_VER_MINOR(ver);
    int patch = XELP_VER_PATCH(ver);

    FILE *fp = stdout;
    if (argc > 1) {
        fp = fopen(argv[1], "w");
        if (!fp) {
            fprintf(stderr, "extract_version: cannot open %s\n", argv[1]);
            return 1;
        }
    }

    fprintf(fp, "# Auto-generated from XELP_VERSION in src/xelp.h -- do not edit\n");
    fprintf(fp, "version_hex: \"0x%08lx\"\n", ver);
    fprintf(fp, "major: %d\n", major);
    fprintf(fp, "minor: %d\n", minor);
    fprintf(fp, "patch: %d\n", patch);

    if (patch == 0)
        fprintf(fp, "version: \"%d.%d\"\n", major, minor);
    else
        fprintf(fp, "version: \"%d.%d.%d\"\n", major, minor, patch);

    if (patch == 0)
        fprintf(fp, "tag: \"v%d.%d\"\n", major, minor);
    else
        fprintf(fp, "tag: \"v%d.%d.%d\"\n", major, minor, patch);

    if (fp != stdout)
        fclose(fp);

    return 0;
}
