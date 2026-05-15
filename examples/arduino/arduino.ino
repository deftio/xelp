/*
 * arduino-example.ino -- Basic xelp example using the raw C API.
 *
 * Demonstrates native argc/argv dispatch:
 *   - echo: argv iteration
 *   - led:  XelpArgvInt for a single integer arg
 *   - divmod: XelpArgvInt + register returns (R1/R2)
 *
 * Works on any Arduino board with a Serial port.
 * Open the Serial Monitor at 115200 baud and type "help".
 *
 * For the C++ Easy API approach, see arduino-cpp-example.
 */

#include "xelp.h"

XELP cli;

/* ------------------------------------------------------------------ */
/* Output callback                                                     */
/* ------------------------------------------------------------------ */

void writeChar(char c) {
    Serial.write(c);
}

/* ------------------------------------------------------------------ */
/* Command handlers (native argc/argv dispatch)                             */
/* ------------------------------------------------------------------ */

XELPRESULT cmdHelp(XELP *ths, int argc, const char **argv) {
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

XELPRESULT cmdBanner(XELP *ths, int argc, const char **argv) {
    (void)ths; (void)argc; (void)argv;
    Serial.println(XELP_BANNER_STR);
    return XELP_S_OK;
}

XELPRESULT cmdEcho(XELP *ths, int argc, const char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (i > 1) XelpOut(ths, " ", 0);
        XelpOut(ths, argv[i], 0);
    }
    XelpOut(ths, "\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdLed(XELP *ths, int argc, const char **argv) {
    int val;
    if (XelpArgvInt(argv, argc, 1, &val) != XELP_S_OK) {
        XelpOut(ths, "usage: led <0|1>\n", 0);
        return XELP_E_ERR;
    }
    digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
    XelpOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdDivmod(XELP *ths, int argc, const char **argv) {
    int a, d;
    if (XelpArgvInt(argv, argc, 1, &a) != XELP_S_OK ||
        XelpArgvInt(argv, argc, 2, &d) != XELP_S_OK) {
        XelpOut(ths, "usage: divmod <a> <b>\n", 0);
        return XELP_E_ERR;
    }
    if (d == 0) {
        XelpOut(ths, "division by zero\n", 0);
        return XELP_E_ERR;
    }
    ths->mR[1] = a / d;
    ths->mR[2] = a % d;
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry gMyCLICommands[] = {
    { &cmdHelp,   "help",   "show help"                        },
    { &cmdBanner, "banner", "print banner"                     },
    { &cmdEcho,   "echo",   "echo <args...>"                   },
    { &cmdLed,    "led",    "led <0|1> -- set LED"             },
    { &cmdDivmod, "divmod", "divmod <a> <b> -- R1=a/b R2=a%%b" },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup() {
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(LED_BUILTIN, OUTPUT);

    XelpInit(&cli, "xelp Arduino example v1.0\n");
    XELP_SET_FN_OUT(cli, &writeChar);
    XELP_SET_FN_CLI(cli, gMyCLICommands);

    Serial.println(XELP_BANNER_STR);

    /* Run a startup script to demonstrate XelpParse + registers */
    XelpParse(&cli, "echo Hello from xelp; divmod 17 5",
              XelpStrLen("echo Hello from xelp; divmod 17 5"));
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        XelpParseKey(&cli, c);
    }
}
