/*
 * scripting-example.c -- Using xelp to execute stored scripts.
 *
 * This example shows the difference between interactive CLI mode
 * (XelpParseKey, character-by-character) and scripting mode
 * (XelpParse / XelpParseXB, run a whole buffer at once).
 *
 * Scripting is useful for:
 *   - Startup configuration stored in ROM/flash
 *   - Macro playback ("run this sequence of commands")
 *   - Batch processing from a file or network packet
 *   - Self-test sequences
 *
 * Scripts support the same syntax as the interactive CLI:
 *   - Semicolons separate commands on one line
 *   - Newlines separate commands across lines
 *   - # starts a comment (rest of line ignored)
 *   - "quoted strings" are single tokens
 *   - Backtick (`) escapes the next character
 *
 * Build (from repo root):
 *   gcc -Wall -Isrc examples/scripting/scripting-example.c src/xelp.c -o scripting-example
 */

#include <stdio.h>
#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform abstraction -- just putchar for this example               */
/* ------------------------------------------------------------------ */

static void out_putc(char c) { putchar(c); }

/* ------------------------------------------------------------------ */
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

static XELP cli;

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

static int g_led_state = 0;

static XELPRESULT cmd_led(XELP *ths, int argc, const char **argv)
{
    int val;
    if (argc > 1) {
        XelpParseNum(argv[1], XelpStrLen(argv[1]), &val);
        g_led_state = val;
        XelpOut(ths, "LED -> ", 0);
        XelpOut(ths, (val ? "ON" : "OFF"), 0);
        XelpOut(ths, "\n", 0);
    }
    return XELP_S_OK;
}

static int g_gain = 50;

static XELPRESULT cmd_gain(XELP *ths, int argc, const char **argv)
{
    int val;
    (void)ths;
    if (argc > 1) {
        XelpParseNum(argv[1], XelpStrLen(argv[1]), &val);
        g_gain = val;
    }
    printf("gain = %d\n", g_gain);
    return XELP_S_OK;
}

static XELPRESULT cmd_status(XELP *ths, int argc, const char **argv)
{
    (void)ths; (void)argc; (void)argv;
    printf("LED: %s, gain: %d\n", g_led_state ? "ON" : "OFF", g_gain);
    return XELP_S_OK;
}

static XELPRESULT cmd_help(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmd_help,   "help",   "show help"                  },
    { &cmd_echo,   "echo",   "echo arguments"             },
    { &cmd_led,    "led",    "led <0|1> -- set LED state"  },
    { &cmd_gain,   "gain",   "gain <N> -- set gain value"  },
    { &cmd_status, "status", "show current state"          },
    XELP_FUNC_ENTRY_LAST
};

/* ================================================================== */
/* SCRIPTING vs INTERACTIVE -- the two ways to drive xelp              */
/* ================================================================== */

int main(void)
{
    printf("%s\n", XELP_BANNER_STR);

    /* --- Initialize ------------------------------------------------ */

    XelpInit(&cli, "Scripting Example v1.0\n");
    XELP_SET_FN_OUT(cli, &out_putc);
    XELP_SET_FN_CLI(cli, commands);

    /* =============================================================== */
    /* PART 1: SCRIPTING MODE                                          */
    /*                                                                  */
    /* XelpParse() executes a whole buffer of commands at once.         */
    /* The buffer is const -- xelp never modifies it, so scripts can   */
    /* live in ROM / flash / .rodata.                                   */
    /* =============================================================== */

    printf("--- Running startup script ---\n");

    /* A typical startup script stored as a const string.
     * This could equally come from EEPROM, a config file, or a
     * network packet -- xelp doesn't care where the bytes come from. */
    {
        const char *startup_script =
            "# Device startup configuration\n"
            "echo Initializing...\n"
            "led 1\n"
            "gain 75\n"
            "echo Startup complete\n"
            "status\n";

        XelpParse(&cli, startup_script, XelpStrLen(startup_script));
    }

    printf("\n--- Running one-liner script (semicolons) ---\n");

    /* Semicolons separate commands on a single line.
     * Handy for macro-style sequences. */
    {
        const char *macro = "echo macro start; led 0; gain 100; status; echo done";
        XelpParse(&cli, macro, XelpStrLen(macro));
    }

    printf("\n--- Running script via XelpParseXB ---\n");

    /* XelpParseXB takes a XelpBuf, which is useful when you already
     * have position-tracked buffers (e.g. from a protocol parser). */
    {
        const char *script =
            "echo Using XelpBuf API\n"
            "gain 50\n"
            "status\n";
        XelpBuf xb;
        XELP_XB_INIT(xb, (char *)script, XelpStrLen(script));
        XelpParseXB(&cli, &xb);
    }

    /* =============================================================== */
    /* PART 2: INTERACTIVE CLI MODE                                    */
    /*                                                                  */
    /* XelpParseKey() processes one character at a time, just like a   */
    /* user typing at a terminal. It handles:                           */
    /*   - Echoing characters                                           */
    /*   - Backspace editing                                            */
    /*   - Dispatching on ENTER                                         */
    /*   - Mode switching (CLI / KEY / THR)                             */
    /*                                                                  */
    /* This is what you'd use in your main loop to handle serial/USB   */
    /* input from a human.                                             */
    /* =============================================================== */

    printf("\n--- Interactive mode (simulated typing) ---\n");

    /* Simulate a user typing "status\n" one character at a time */
    {
        const char *typed = "status\n";
        int i;
        for (i = 0; i < XelpStrLen(typed); i++) {
            XelpParseKey(&cli, typed[i]);
        }
    }

    /* =============================================================== */
    /* SUMMARY                                                         */
    /*                                                                  */
    /*   XelpParse / XelpParseXB  =  scripting (batch, ROM, config)   */
    /*   XelpParseKey             =  interactive (terminal, serial)    */
    /*                                                                  */
    /* Both use the same command table. Both are non-destructive        */
    /* (the input buffer is never modified). Both can be used on the    */
    /* same XELP instance.                                             */
    /* =============================================================== */

    printf("\n--- Done ---\n");
    return 0;
}
