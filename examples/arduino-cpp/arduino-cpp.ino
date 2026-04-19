/*
 * arduino-cpp-example.ino -- Arduino example using the XelpCLI C++ wrapper.
 *
 * Demonstrates:
 *   - begin() + setCommands() + setPrompt() setup
 *   - poll(Serial) in loop() to handle interactive input
 *   - run() for executing a startup script
 *
 * The wrapper eliminates boilerplate: no manual XELP_SET_FN_* macros,
 * no Serial.available() loop -- just call poll(Serial).
 *
 * Multi-instance: create a second XelpCLI for Serial2 (or any Stream).
 *   XelpCLI cli2;
 *   cli2.begin("Port 2", &writeChar2);
 *   cli2.poll(Serial2);
 */

#include "xelp.h"
#include "XelpArduino.h"

/* ------------------------------------------------------------------ */
/* Output callback -- one per serial port                              */
/* ------------------------------------------------------------------ */

void myOutput(char c) { Serial.write(c); }

/* ------------------------------------------------------------------ */
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

XelpCLI cli;

XELPRESULT cmdHelp(XELP *ths, const char* args, int len)
{
    (void)args; (void)len;
    return XELPHelp(ths);
}

XELPRESULT cmdEcho(XELP *ths, const char* args, int len)
{
    XelpBuf b, tok;
    int i, n;
    XELP_XBInit(b, (char*)args, len);
    XELPNumToks(&b, &n);
    for (i = 1; i < n; i++) {
        XELP_XBTOP(b);
        if (XELPTokN(&b, i, &tok) == XELP_S_OK) {
            if (i > 1) XELPOut(ths, " ", 0);
            XELPOut(ths, tok.s, (int)(tok.p - tok.s));
        }
    }
    XELPOut(ths, "\n", 0);
    return XELP_S_OK;
}

static int gLedPin = LED_BUILTIN;

XELPRESULT cmdLed(XELP *ths, const char* args, int len)
{
    XelpBuf b, tok;
    int val;
    XELP_XBInit(b, (char*)args, len);
    if (XELPTokN(&b, 1, &tok) == XELP_S_OK) {
        XELPParseNum(tok.s, (int)(tok.p - tok.s), &val);
        digitalWrite(gLedPin, val ? HIGH : LOW);
        XELPOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    }
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp, "help", "show help"                },
    { &cmdEcho, "echo", "echo arguments"           },
    { &cmdLed,  "led",  "led <0|1> -- set LED"     },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(gLedPin, OUTPUT);

    cli.begin("Arduino C++ xelp example v1.0\n", &myOutput);
    cli.setCommands(commands);
    cli.setPrompt("arduino>");

    /* Run a startup script */
    cli.run("echo Hello from xelp; led 1");

    Serial.println();
}

/* ------------------------------------------------------------------ */
/* Loop -- just poll                                                   */
/* ------------------------------------------------------------------ */

void loop()
{
    cli.poll(Serial);
}
