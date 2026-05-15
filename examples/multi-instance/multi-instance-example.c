/*
 * multi-instance-example.c -- Running two xelp instances on different ports.
 *
 * Demonstrates xelp's multi-instance capability: each XELP struct holds
 * its own state, so you can attach independent CLIs to different UARTs,
 * USB CDC endpoints, BLE characteristics, etc.
 *
 * In this example:
 *   - cli_a listens on UART0 (the debug console)
 *   - cli_b listens on UART1 (a field service port)
 *
 * Each has its own prompt, its own command table, and its own mode state.
 * They share the same C functions but maintain separate mR[] registers.
 */

#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform stubs -- replace with your hardware                        */
/* ------------------------------------------------------------------ */

static void uart0_putc(char c) { (void)c; /* UART0 TX */ }
static void uart1_putc(char c) { (void)c; /* UART1 TX */ }

static void uart0_bksp(void) { uart0_putc('\b'); uart0_putc(' '); uart0_putc('\b'); }
static void uart1_bksp(void) { uart1_putc('\b'); uart1_putc(' '); uart1_putc('\b'); }

/* ------------------------------------------------------------------ */
/* Shared commands -- ths points to whichever instance called them      */
/* ------------------------------------------------------------------ */

XELP cli_a;
XELP cli_b;

static XELPRESULT cmd_help(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

static XELPRESULT cmd_status(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    /* ths points to whichever instance called this command,
     * so output goes to the correct UART automatically. */
    XelpOut(ths, "System OK\n", 0);
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command tables -- can be different per instance                     */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands_a[] = {
    { &cmd_help,   "help",   "show help"       },
    { &cmd_status, "status", "system status"   },
    XELP_FUNC_ENTRY_LAST
};

XELPCLIFuncMapEntry commands_b[] = {
    { &cmd_help,   "help",   "show help"       },
    { &cmd_status, "status", "system status"   },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

void main(void)
{
    /* Initialize two independent instances */
    XelpInit(&cli_a, "Debug Console (UART0)");
    XelpInit(&cli_b, "Service Port (UART1)");

    /* Each gets its own output and backspace handler */
    XELP_SET_FN_OUT(cli_a, &uart0_putc);
    XELP_SET_FN_BKSP(cli_a, &uart0_bksp);
    XELP_SET_FN_CLI(cli_a, commands_a);

    XELP_SET_FN_OUT(cli_b, &uart1_putc);
    XELP_SET_FN_BKSP(cli_b, &uart1_bksp);
    XELP_SET_FN_CLI(cli_b, commands_b);

    /* Each can have its own prompt */
    XELP_SET_VAL_CLI_PROMPT(cli_a, "dbg>");
    XELP_SET_VAL_CLI_PROMPT(cli_b, "svc>");

    /* Show startup on both */
    XelpOut(&cli_a, XELP_BANNER_STR, 0);
    XelpParseKey(&cli_a, '\n');

    XelpOut(&cli_b, "Service port ready.\n", 0);
    XelpParseKey(&cli_b, '\n');

    /* Main loop -- poll both UARTs */
    for (;;) {
        /* if (uart0_rx_ready()) XelpParseKey(&cli_a, uart0_getc()); */
        /* if (uart1_rx_ready()) XelpParseKey(&cli_b, uart1_getc()); */
    }
}
