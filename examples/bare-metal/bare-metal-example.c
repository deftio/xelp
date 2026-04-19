/*
 * bare-metal-example.c -- Minimal xelp example for bare-metal targets.
 *
 * Demonstrates CLI mode, KEY mode, and THR mode on a system with a single
 * UART. No OS, no stdlib, no ncurses. This is the starting point for
 * porting xelp to a new microcontroller.
 *
 * Replace the UART stubs with your hardware-specific code.
 */

#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform stubs -- replace with your hardware                        */
/* ------------------------------------------------------------------ */

static void uart_putc(char c)
{
    /* TODO: write c to UART TX register */
    /* while (!UART_TX_READY); UART_TX_REG = c; */
    (void)c;
}

static void uart_bksp(void)
{
    uart_putc('\b');
    uart_putc(' ');
    uart_putc('\b');
}

static int uart_rx_ready(void)
{
    /* TODO: return nonzero if a byte is available */
    return 0;
}

static char uart_getc(void)
{
    /* TODO: return the received byte */
    return 0;
}

/* ------------------------------------------------------------------ */
/* KEY mode commands (single keypress, no ENTER)                       */
/* ------------------------------------------------------------------ */

static XELPRESULT key_help(XELP *ths, int c)
{
    (void)c;
    return XELPHelp(ths);
}

static XELPRESULT key_banner(XELP *ths, int c)
{
    (void)c;
    XELPOut(ths, XELP_BANNER_STR, 0);
    return XELP_S_OK;
}

XELPKeyFuncMapEntry key_commands[] = {
    { &key_help,   '?', "show help"     },
    { &key_banner, 'b', "print banner"  },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* CLI mode commands (type command, press ENTER)                        */
/* ------------------------------------------------------------------ */

static XELPRESULT cmd_help(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    return XELPHelp(ths);
}

static XELPRESULT cmd_echo(XELP *ths, const char *args, int len)
{
    XELPOut(ths, args, len);
    XELPOut(ths, "\n", 0);
    return XELP_S_OK;
}

static XELPRESULT cmd_info(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    XELPOut(ths, "XELP size: ", 0);
    /* On a real target you'd format sizeof(XELP) here */
    XELPOut(ths, " bytes\n", 0);
    return XELP_S_OK;
}

XELPCLIFuncMapEntry cli_commands[] = {
    { &cmd_help, "help", "show help"            },
    { &cmd_echo, "echo", "echo args back"       },
    { &cmd_info, "info", "show system info"     },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* THR mode pass-through (redirect keys to another peripheral)         */
/* ------------------------------------------------------------------ */

static void thr_passthrough(char c)
{
    /* TODO: forward c to a second UART (modem, debug port, etc.) */
    (void)c;
}

/* ------------------------------------------------------------------ */
/* Mode change callback (optional)                                     */
/* ------------------------------------------------------------------ */

static void on_mode_change(int mode)
{
    extern XELP cli;
    if (mode == XELP_MODE_CLI)
        XELPOut(&cli, "[CLI mode]\n", 0);
    else if (mode == XELP_MODE_KEY)
        XELPOut(&cli, "[KEY mode]\n", 0);
    else if (mode == XELP_MODE_THR)
        XELPOut(&cli, "[THR mode]\n", 0);
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

XELP cli;

void main(void)
{
    XELPInit(&cli, "Bare Metal Example v1.0\n");

    /* Wire up the platform abstraction layer */
    XELP_SET_FN_OUT(cli, &uart_putc);
    XELP_SET_FN_BKSP(cli, &uart_bksp);
    XELP_SET_FN_THR(cli, &thr_passthrough);
    XELP_SET_FN_EMCHG(cli, &on_mode_change);

    /* Register command tables */
    XELP_SET_FN_KEY(cli, key_commands);
    XELP_SET_FN_CLI(cli, cli_commands);

    /* Print startup banner and help */
    XELPOut(&cli, XELP_BANNER_STR, 0);
    XELPHelp(&cli);

    /* Show initial prompt */
    XELPParseKey(&cli, '\n');

    /* Main loop */
    for (;;) {
        if (uart_rx_ready()) {
            XELPParseKey(&cli, uart_getc());
        }
    }
}
