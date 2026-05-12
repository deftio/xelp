/*
  fuzz_buf2argv.c — libFuzzer harness for XelpBuf2Argv

  Feeds raw fuzz input to XelpBuf2Argv and exercises the
  XelpArgvInt / XelpArgvStr accessors with fuzz-derived indices.
  Also calls with restricted maxargs values to exercise error paths.

  Build (requires clang with libFuzzer):
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Isrc \
          src/xelp.c tests/fuzz/fuzz_buf2argv.c -o build/fuzz_buf2argv

  Run:
    ./build/fuzz_buf2argv tests/fuzz/corpus_buf2argv -max_total_time=60
*/

#include <stdint.h>
#include <stddef.h>
#include "xelp.h"

/* --- Null-sink callbacks ------------------------------------------------- */

static void fuzz_out(char c)       { (void)c; }
static void fuzz_bksp(void)        { }

static XELPRESULT fuzz_cli_fn(XELP *ths, const char *args, int max) {
    (void)ths; (void)args; (void)max;
    return XELP_S_OK;
}

static XELPCLIFuncMapEntry fuzz_cli_cmds[] = {
    { &fuzz_cli_fn, "echo", "echo" },
    { &fuzz_cli_fn, "set",  "set"  },
    XELP_FUNC_ENTRY_LAST
};

/* --- Harness ------------------------------------------------------------- */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    XELP x;
    char *argv[XELP_ARGV_MAX];
    int argc, val, slen;
    const char *s;
    signed char idx;

    XelpInit(&x, "fuzz");
    XELP_SET_FN_OUT(x, &fuzz_out);
    XELP_SET_FN_CLI(x, fuzz_cli_cmds);
    XELP_SET_FN_BKSP(x, &fuzz_bksp);

    if (size == 0) return 0;

    /* Use first byte as a fuzz-derived index (can be negative via signed char) */
    idx = (signed char)data[0];

    /* Full-capacity call */
    if (XelpBuf2Argv(&x, (const char *)data, (int)size, &argc, argv, XELP_ARGV_MAX) == XELP_S_OK) {
        /* Exercise accessor functions with fuzz-derived index */
        XelpArgvInt(argv, argc, (int)idx, &val);
        XelpArgvStr(argv, argc, (int)idx, &s, &slen);
        /* Also try valid range if argc > 0 */
        if (argc > 0) {
            XelpArgvInt(argv, argc, 0, &val);
            XelpArgvStr(argv, argc, argc - 1, &s, &slen);
        }
    }

    /* Exercise with maxargs=1 (error path if input has multiple tokens) */
    XelpBuf2Argv(&x, (const char *)data, (int)size, &argc, argv, 1);

    /* Exercise with maxargs=2 */
    XelpBuf2Argv(&x, (const char *)data, (int)size, &argc, argv, 2);

    return 0;
}
