/*
 * posix-argv-example.c -- Demonstrates native argc/argv argument parsing.
 *
 * Non-interactive, printf-based (no ncurses). Shows the native
 * argc/argv dispatch with XelpArgvInt/XelpArgvStr helpers.
 *
 * Build (from repo root):
 *   gcc -Wall -Isrc examples/posix-argv/posix-argv-example.c src/xelp.c \
 *       -o posix-argv-example
 */

#include <stdio.h>
#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform abstraction -- just putchar                                 */
/* ------------------------------------------------------------------ */

static void out_putc(char c) { putchar(c); }

/* ------------------------------------------------------------------ */
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

static XELP cli;

/*
 * echo -- print argv[1..n-1] separated by spaces.
 * Demonstrates native argc/argv for simple multi-arg handling.
 */
static XELPRESULT cmd_echo(XELP *ths, int argc, const char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (i > 1) XelpOut(ths, " ", 0);
        XelpOut(ths, argv[i], 0);
    }
    XelpOut(ths, "\n", 0);
    return XELP_S_OK;
}

/*
 * divmod -- compute quotient and remainder using native argc/argv + XelpArgvInt.
 * Usage: divmod <a> <b>
 * Sets R1 = a / b, R2 = a % b.
 */
static XELPRESULT cmd_divmod(XELP *ths, int argc, const char **argv)
{
    int a, b;

    if (argc < 3) {
        XelpOut(ths, "usage: divmod <a> <b>\n", 0);
        return XELP_E_ERR;
    }
    if (XelpArgvInt(argv, argc, 1, &a) != XELP_S_OK ||
        XelpArgvInt(argv, argc, 2, &b) != XELP_S_OK || b == 0) {
        XelpOut(ths, "error: need two non-zero integers\n", 0);
        return XELP_E_ERR;
    }
    ths->mR[1] = a / b;
    ths->mR[2] = a % b;
    printf("  divmod(%d, %d) => quotient=%d remainder=%d\n",
           a, b, (int)ths->mR[1], (int)ths->mR[2]);
    return XELP_S_OK;
}

/*
 * divmod_old -- same as divmod but using XelpArgs (for comparison).
 * Demonstrates the traditional XelpArgs sequential iterator approach.
 */
static XELPRESULT cmd_divmod_old(XELP *ths, int argc, const char **argv)
{
    int a, b;

    if (argc < 3) {
        XelpOut(ths, "usage: divmod_old <a> <b>\n", 0);
        return XELP_E_ERR;
    }
    XelpParseNum(argv[1], XelpStrLen(argv[1]), &a);
    XelpParseNum(argv[2], XelpStrLen(argv[2]), &b);
    if (b == 0) {
        XelpOut(ths, "usage: divmod_old <a> <b>\n", 0);
        return XELP_E_ERR;
    }
    ths->mR[1] = a / b;
    ths->mR[2] = a % b;
    printf("  divmod_old(%d, %d) => quotient=%d remainder=%d\n",
           a, b, (int)ths->mR[1], (int)ths->mR[2]);
    return XELP_S_OK;
}

/*
 * set -- multi-arg "set key value" using native argc/argv + XelpArgvStr.
 * Demonstrates string access via argv.
 */
static XELPRESULT cmd_set(XELP *ths, int argc, const char **argv)
{
    const char *key, *val;
    int klen, vlen;

    if (argc < 3) {
        XelpOut(ths, "usage: set <key> <value>\n", 0);
        return XELP_E_ERR;
    }
    XelpArgvStr(argv, argc, 1, &key, &klen);
    XelpArgvStr(argv, argc, 2, &val, &vlen);
    printf("  set: key=\"%s\" value=\"%s\"\n", key, val);
    return XELP_S_OK;
}

/*
 * help -- wraps XelpHelp.
 */
static XELPRESULT cmd_help(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmd_help,       "help",       "show help"                          },
    { &cmd_echo,       "echo",       "echo args (native argc/argv style)"    },
    { &cmd_divmod,     "divmod",     "divmod <a> <b> (Buf2Argv + ArgvInt)" },
    { &cmd_divmod_old, "divmod_old", "divmod <a> <b> (XelpArgs style)"   },
    { &cmd_set,        "set",        "set <key> <value> (ArgvStr)"       },
    XELP_FUNC_ENTRY_LAST
};

/* ================================================================== */
/* main                                                                */
/* ================================================================== */

int main(void)
{
    printf("%s\n", XELP_BANNER_STR);

    /* --- Initialize ------------------------------------------------ */

    XelpInit(&cli, "posix-argv example v1.0\n");
    XELP_SET_FN_OUT(cli, &out_putc);
    XELP_SET_FN_CLI(cli, commands);

    /* =============================================================== */
    /* PART 1: Script execution via XelpParse                          */
    /* =============================================================== */

    printf("--- Part 1: Script execution ---\n\n");

    {
        const char *script =
            "# Startup script using native argc/argv commands\n"
            "echo Initializing device...\n"
            "divmod 17 5\n"
            "divmod_old 17 5\n"
            "set mode fast\n"
            "echo Startup complete\n";

        XelpParse(&cli, script, XelpStrLen(script));
    }

    printf("\n--- Registers after divmod: R1=%d R2=%d ---\n",
           (int)XELP_R1(cli), (int)XELP_R2(cli));

    /* =============================================================== */
    /* PART 2: Interactive simulation via XelpParseKey                  */
    /* =============================================================== */

    printf("\n--- Part 2: Interactive simulation ---\n\n");

    {
        const char *typed = "divmod 17 5\n";
        int i;
        for (i = 0; typed[i]; i++)
            XelpParseKey(&cli, typed[i]);
    }

    /* =============================================================== */
    /* PART 3: Direct argv helpers                                      */
    /*                                                                  */
    /* XelpArgvInt / XelpArgvStr provide bounds-checked access to the   */
    /* argv array that the dispatch engine provides to every handler.   */
    /* =============================================================== */

    /* =============================================================== */
    /* SUMMARY                                                         */
    /*                                                                  */
    /* Command handlers receive argc/argv directly from the dispatch   */
    /* engine. XelpArgvInt and XelpArgvStr provide bounds-checked      */
    /* access to individual arguments.                                 */
    /* =============================================================== */

    printf("\n--- Done ---\n");
    return 0;
}
