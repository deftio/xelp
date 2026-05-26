/*
  fuzz_script.c — libFuzzer harness for XELP Script evaluator

  Feeds the entire fuzz input as a script buffer to the script evaluator.
  Exercises variable set/get, math/comparison/logic builtins, paren
  evaluation, label/goto, _if/_next, and _func/_return.

  Build (requires clang with libFuzzer):
    clang -fsanitize=fuzzer,address,undefined -g -O1 -DXELP_ENABLE_SCRIPT \
          -Isrc src/xelp.c tests/fuzz/fuzz_script.c -o build/fuzz_script

  Run:
    mkdir -p tests/fuzz/corpus_script/generated
    cp -n tests/fuzz/corpus_script/seeds/* tests/fuzz/corpus_script/generated/
    ./build/fuzz_script tests/fuzz/corpus_script/generated -max_total_time=60
*/

#include <stdint.h>
#include <stddef.h>
#include "xelp.h"

/* --- Null-sink callbacks ------------------------------------------------- */

static void fuzz_out(char c)       { (void)c; }
static void fuzz_emchg(int mode)   { (void)mode; }

static int gBudget;

static XELPRESULT fuzz_breakpoint(XELP *ths) {
    (void)ths;
    if (--gBudget <= 0) return XELP_E_BUDGET;
    return XELP_S_OK;
}

static XELPRESULT fuzz_cli_fn(XELP *ths, int argc, const char **argv) {
    (void)ths; (void)argc; (void)argv;
    XelpSetResultInt(ths, 42);
    return XELP_S_OK;
}

static XELPRESULT fuzz_def_cli(XELP *ths, int argc, const char **argv) {
    (void)ths; (void)argc; (void)argv;
    return XELP_E_CMDNOTFOUND;
}

/* --- Command tables ------------------------------------------------------ */

static XELPKeyFuncMapEntry fuzz_key_cmds[] = {
    XELP_FUNC_ENTRY_LAST
};

static XELPCLIFuncMapEntry fuzz_cli_cmds[] = {
    { &fuzz_cli_fn, "echo", "echo" },
    { &fuzz_cli_fn, "test", "test" },
    { &fuzz_cli_fn, "read_adc", "adc" },
    XELP_FUNC_ENTRY_LAST
};

/* --- Harness ------------------------------------------------------------- */

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    XELP x;

    if (size > 256) size = 256; /* bound input to prevent timeout */

    XelpInit(&x, "fuzz");
    XELP_SET_FN_OUT(x, &fuzz_out);
    XELP_SET_FN_CLI(x, fuzz_cli_cmds);
    XELP_SET_FN_KEY(x, fuzz_key_cmds);
    XELP_SET_FN_DEF_CLI(x, &fuzz_def_cli);
    XELP_SET_FN_EMCHG(x, &fuzz_emchg);
    XELP_SET_FN_BREAKPOINT(x, &fuzz_breakpoint);

    gBudget = 500; /* max 500 statements per fuzz input */

    XelpParse(&x, (const char *)data, (int)size);

    /* Also exercise XelpCallProc if input starts with a letter */
    if (size > 4 && data[0] >= 'a' && data[0] <= 'z') {
        char buf[64];
        int i;
        int len = (size < 63) ? (int)size : 63;
        for (i = 0; i < len; i++) buf[i] = (char)data[i];
        buf[len] = '\0';
        gBudget = 100;
        XelpCallProc(&x, buf);
    }

    return 0;
}
