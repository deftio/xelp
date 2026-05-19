/*
  fuzz_parse.c — libFuzzer harness for XelpParse

  Feeds the entire fuzz input as a command buffer to XelpParse.
  Exercises the PSM tokenizer, command dispatch, semicolon/newline
  splitting, argument parsing, and XelpParseNum.

  Build (requires clang with libFuzzer):
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Isrc \
          src/xelp.c tests/fuzz/fuzz_parse.c -o build/fuzz_parse

  Run:
    mkdir -p tests/fuzz/corpus_parse/generated
    cp -n tests/fuzz/corpus_parse/seeds/* tests/fuzz/corpus_parse/generated/
    ./build/fuzz_parse tests/fuzz/corpus_parse/generated -max_total_time=60
*/

#include <stdint.h>
#include <stddef.h>
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

static XELPRESULT fuzz_cli_fn(XELP *ths, int argc, const char **argv) {
    const char *s;
    int v, slen;
    (void)ths;

    XelpArgvInt(argv, argc, 0, &v);
    if (argc > 1)
        XelpArgvStr(argv, argc, 1, &s, &slen);
    return XELP_S_OK;
}

static XELPRESULT fuzz_def_cli(XELP *ths, int argc, const char **argv) {
    int v;
    (void)ths;
    XelpArgvInt(argv, argc, argc - 1, &v);
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
    XelpInit(&x, "fuzz");

    XELP_SET_FN_OUT(x, &fuzz_out);
    XELP_SET_FN_CLI(x, fuzz_cli_cmds);
    XELP_SET_FN_KEY(x, fuzz_key_cmds);
    XELP_SET_FN_THR(x, &fuzz_thr);
    XELP_SET_FN_BKSP(x, &fuzz_bksp);
    XELP_SET_FN_DEF_CLI(x, &fuzz_def_cli);
    XELP_SET_FN_DEF_KEY(x, &fuzz_def_key);
    XELP_SET_FN_EMCHG(x, &fuzz_emchg);

    XelpParse(&x, (const char *)data, (int)size);

    return 0;
}
