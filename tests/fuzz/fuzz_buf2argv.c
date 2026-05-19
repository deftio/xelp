/*
  fuzz_buf2argv.c — libFuzzer harness for argc/argv tokenization behind dispatch

  v0.4.0: `XelpBuf2Argv()` is internal (`_xelpBuf2Argv` in xelp.c). This harness
  drives that path indirectly via XelpParse() so quotes, escapes, overflow
  semantics, and XelpArgvInt / XelpArgvStr stay under fuzz.

  Build (requires clang with libFuzzer):
    clang -fsanitize=fuzzer,address,undefined -g -O1 -Isrc \
          src/xelp.c tests/fuzz/fuzz_buf2argv.c -o build/fuzz_buf2argv

  Run:
    mkdir -p tests/fuzz/corpus_buf2argv/generated
    cp -n tests/fuzz/corpus_buf2argv/seeds/* tests/fuzz/corpus_buf2argv/generated/
    ./build/fuzz_buf2argv tests/fuzz/corpus_buf2argv/generated -max_total_time=60
*/

#include <stdint.h>
#include <stddef.h>
#include "xelp.h"

static void fuzz_out(char c) { (void)c; }

static signed char fuzz_idx_storage;

static void fuzz_exercise_argv(int argc, const char **argv) {
    const char *s;
    int val, slen;

    XelpArgvInt(argv, argc, (int)fuzz_idx_storage, &val);
    XelpArgvStr(argv, argc, (int)fuzz_idx_storage, &s, &slen);
    if (argc > 0) {
        XelpArgvInt(argv, argc, 0, &val);
        XelpArgvStr(argv, argc, argc - 1, &s, &slen);
        /* XelpParseNum on argv[1] when present */
        if (argc > 1)
            XelpParseNum(argv[1], XelpStrLen(argv[1]), &val);
    }
}

static XELPRESULT fuzz_argv_cmd(XELP *ths, int argc, const char **argv) {
    (void)ths;
    fuzz_exercise_argv(argc, argv);
    return XELP_S_OK;
}

static XELPRESULT fuzz_argv_def_cli(XELP *ths, int argc, const char **argv) {
    (void)ths;
    fuzz_exercise_argv(argc, argv);
    return XELP_S_OK;
}

static XELPCLIFuncMapEntry fuzz_cli_cmds[] = {
    { &fuzz_argv_cmd, "echo", "echo" },
    { &fuzz_argv_cmd, "set",  "set"  },
    XELP_FUNC_ENTRY_LAST
};

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    XELP x;

    XelpInit(&x, "fuzz");
    XELP_SET_FN_OUT(x, &fuzz_out);
    XELP_SET_FN_CLI(x, fuzz_cli_cmds);
    XELP_SET_FN_DEF_CLI(x, &fuzz_argv_def_cli);

    if (size == 0)
        return 0;

    fuzz_idx_storage = (signed char)data[0];

    XelpParse(&x, (const char *)(data + 1), (int)(size - 1));

    return 0;
}
