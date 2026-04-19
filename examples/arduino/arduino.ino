/*
 * arduino-example.ino -- Basic xelp example using the raw C API.
 *
 * Works on any Arduino board with a Serial port. Provides an
 * interactive CLI over Serial with help, echo, and LED control.
 *
 * Open the Serial Monitor at 115200 baud and type "help".
 *
 * For the simpler C++ wrapper approach, see arduino-cpp-example.
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
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

XELPRESULT cmdHelp(XELP *ths, const char* args_str, int maxlen) {
    (void)args_str; (void)maxlen;
    return XELPHelp(ths);
}

XELPRESULT cmdBanner(XELP *ths, const char* args_str, int maxlen) {
    (void)ths; (void)args_str; (void)maxlen;
    Serial.println(XELP_BANNER_STR);
    return XELP_S_OK;
}

XELPRESULT cmdLed(XELP *ths, const char* args_str, int maxlen) {
    XelpBuf b, tok;
    int val;
    XELP_XBInit(b, (char*)args_str, maxlen);
    if (XELPTokN(&b, 1, &tok) == XELP_S_OK) {
        XELPParseNum(tok.s, (int)(tok.p - tok.s), &val);
        digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
        XELPOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdListToks(XELP *ths, const char* args_str, int maxlen) {
    XelpBuf b, tok;
    int n, i;
    XELP_XBInit(b, (char*)args_str, maxlen);
    XELPNumToks(&b, &n);
    Serial.print("[");
    Serial.print(n);
    Serial.print("]");
    for (i = 0; i < n; i++) {
        XELP_XBTOP(b);
        XELPTokN(&b, i, &tok);
        XELPOut(ths, "<", -1);
        Serial.print(i);
        XELPOut(ths, ":", -1);
        XELPOut(ths, tok.s, tok.p - tok.s);
        XELPOut(ths, "> ", -1);
    }
    Serial.print("\n");
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry gMyCLICommands[] = {
    { &cmdHelp,     "help",   "show help"            },
    { &cmdBanner,   "banner", "print banner"         },
    { &cmdLed,      "led",    "led <0|1> -- set LED" },
    { &cmdListToks, "lt",     "list tokens"          },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup() {
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(LED_BUILTIN, OUTPUT);

    XELPInit(&cli, "xelp Arduino example v1.0\n");
    XELP_SET_FN_OUT(cli, &writeChar);
    XELP_SET_FN_CLI(cli, gMyCLICommands);

    Serial.println(XELP_BANNER_STR);
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop() {
    if (Serial.available() > 0) {
        char c = Serial.read();
        XELPParseKey(&cli, c);
    }
}
