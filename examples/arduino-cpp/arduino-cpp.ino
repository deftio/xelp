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
    return XelpHelp(ths);
}

XELPRESULT cmdEcho(XELP *ths, const char* args, int len)
{
    XelpBuf b, tok;
    int i, n;
    XELP_XB_INIT(b, (char*)args, len);
    XelpNumToks(&b, &n);
    for (i = 1; i < n; i++) {
        XELP_XB_TOP(b);
        if (XelpTokN(&b, i, &tok) == XELP_S_OK) {
            if (i > 1) XelpOut(ths, " ", 0);
            XelpOut(ths, tok.s, (int)(tok.p - tok.s));
        }
    }
    XelpOut(ths, "\n", 0);
    return XELP_S_OK;
}

static int gLedPin = LED_BUILTIN;

XELPRESULT cmdLed(XELP *ths, const char* args, int len)
{
    XelpBuf b, tok;
    int val;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        XelpParseNum(tok.s, (int)(tok.p - tok.s), &val);
        digitalWrite(gLedPin, val ? HIGH : LOW);
        XelpOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    }
    return XELP_S_OK;
}

/* Integer division: quotient -> R1, remainder -> R2 */
XELPRESULT cmdDivmod(XELP *ths, const char *args, int len)
{
    XelpBuf b, tok;
    int a, d;
    XELP_XB_INIT(b, (char*)args, len);

    XELP_XB_TOP(b);
    if (XelpTokN(&b, 1, &tok) != XELP_S_OK) goto usage;
    a = XelpStr2Int(tok.s, (int)(tok.p - tok.s));

    XELP_XB_TOP(b);
    if (XelpTokN(&b, 2, &tok) != XELP_S_OK) goto usage;
    d = XelpStr2Int(tok.s, (int)(tok.p - tok.s));

    if (d == 0) {
        XelpOut(ths, "division by zero\n", 0);
        return XELP_E_ERR;
    }

    ths->mR[1] = a / d;
    ths->mR[2] = a % d;
    return XELP_S_OK;

usage:
    XelpOut(ths, "usage: divmod <a> <b>\n", 0);
    return XELP_E_ERR;
}

/* Print all 4 registers via the C++ wrapper accessors */
XELPRESULT cmdPrintR(XELP *ths, const char *args, int len)
{
    char buf[64];
    (void)args; (void)len;
    snprintf(buf, sizeof(buf), "R0=%d R1=%d R2=%d R3=%d\n",
             XELP_R0(*ths), XELP_R1(*ths), XELP_R2(*ths), XELP_R3(*ths));
    XelpOut(ths, buf, 0);
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,   "help",   "show help"                       },
    { &cmdEcho,   "echo",   "echo arguments"                  },
    { &cmdLed,    "led",    "led <0|1> -- set LED"            },
    { &cmdDivmod, "divmod", "divmod <a> <b> -- R1=a/b R2=a%b" },
    { &cmdPrintR, "pr",     "print all registers"             },
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

    /* Demonstrate registers: run divmod and read results via C++ accessors.
       r0() is the command status, r1()-r3() hold command-specific returns. */
    cli.run("divmod 17 5");
    Serial.print("17/5 = ");
    Serial.print(cli.r1());       /* quotient  -> 3 */
    Serial.print(" remainder ");
    Serial.println(cli.r2());     /* remainder -> 2 */

    Serial.println();
}

/* ------------------------------------------------------------------ */
/* Loop -- just poll                                                   */
/* ------------------------------------------------------------------ */

void loop()
{
    cli.poll(Serial);
}
