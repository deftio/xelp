/*
  fuzz_parse.c — libFuzzer harness for XELPParse

  Feeds the entire fuzz input as a command buffer to XELPParse.
  Exercises the PSM tokenizer, command dispatch, semicolon/newline
  splitting, argument parsing, and XELPParseNum.

  Build (requires clang with libFuzzer):
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Isrc \
          src/xelp.c tests/fuzz/fuzz_parse.c -o build/fuzz_parse

  Run:
    ./build/fuzz_parse tests/fuzz/corpus_parse -max_total_time=60
*/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "xelp.h"

/* --- Null-sink callbacks ------------------------------------------------- */

static void fuzz_out(char c)       { (void)c; }
static void fuzz_thr(char c)       { (void)c; }
static void fuzz_bksp(void)        { }
static void fuzz_emchg(int mode)   { (void)mode; }

static XELPRESULT fuzz_def_key(XELP *ths, XELPKEYCODE k) {
    (void)ths; (void)k;
    return XELP_S_OK;
}

static XELPRESULT fuzz_cli_fn(XELP *ths, const char *args, int max) {
    (void)ths; (void)args; (void)max;
    return XELP_S_OK;
}

static XELPRESULT fuzz_def_cli(XELP *ths, const char *args, int max) {
    (void)ths; (void)args; (void)max;
    return XELP_S_OK;
}

/* --- Command tables ------------------------------------------------------ */

static XELPKeyFuncMapEntry fuzz_key_cmds[] = {
    XELP_FUNC_ENTRY_LAST
};

static XELPCLIFuncMapEntry fuzz_cli_cmds[] = {
    { &fuzz_cli_fn, "echo", "echo" },
    { &fuzz_cli_fn, "help", "help" },
    { &fuzz_cli_fn, "ver",  "ver"  },
    { &fuzz_cli_fn, "foo",  "foo"  },
    { &fuzz_cli_fn, "bar",  "bar"  },
    XELP_FUNC_ENTRY_LAST
};

/* --- Harness ------------------------------------------------------------- */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    XELP x;
    XELPInit(&x, "fuzz");

    XELP_SET_FN_OUT(x, &fuzz_out);
    XELP_SET_FN_CLI(x, fuzz_cli_cmds);
    XELP_SET_FN_KEY(x, fuzz_key_cmds);
    XELP_SET_FN_THR(x, &fuzz_thr);
    XELP_SET_FN_BKSP(x, &fuzz_bksp);
    XELP_SET_FN_DEF_CLI(x, &fuzz_def_cli);
    XELP_SET_FN_DEF_KEY(x, &fuzz_def_key);
    XELP_SET_FN_EMCHG(x, &fuzz_emchg);

    XELPParse(&x, (const char *)data, (int)size);

    return 0;
}
