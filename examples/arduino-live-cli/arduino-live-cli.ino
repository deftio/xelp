/*
 * arduino-live-cli.ino -- Interactive hardware CLI for Arduino boards.
 *
 * Drive pins, read sensors, and play tones from any serial terminal.
 * Works on Uno, Nano, Mega 2560, Leonardo, and compatible boards.
 *
 * Open Serial Monitor at 115200 baud and type "help".
 *
 * Script demo -- blink the onboard LED from a one-liner:
 *   pinmode 13 out; setpin 13 1; delay 500; setpin 13 0; delay 500; setpin 13 1
 *
 * Command history: use up/down arrow keys to recall previous commands.
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee <deftio [at] deftio [dot] com>
 * BSD-2-Clause -- see LICENSE.txt
 */

#include "xelp.h"

/* Boards with very limited SRAM get a reduced command set. */
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
#define XELP_SMALL_TARGET 1
#endif

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

XELP cli;

void myOutput(char c) {
    if (c == '\n') Serial.write('\r');  /* raw terminals need \r\n */
    Serial.write(c);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Warn (but don't block) if pin is Serial RX/TX. */
static void warnSerial(int pin) {
    if (pin == 0 || pin == 1)
        Serial.println(F("  WARNING: pin 0/1 is Serial RX/TX"));
}

/* Free SRAM (AVR only). */
#if defined(__AVR__)
static int freeMemory() {
    extern int __heap_start, *__brkval;
    int v;
    return (int)&v - (__brkval == 0
        ? (int)&__heap_start : (int)__brkval);
}
#else
static int freeMemory() { return -1; }
#endif

/* Extract token N as a null-terminated string into buf (returns buf). */
static char* tokStr(const char* args, int len, int n, char* buf, int bsz) {
    XelpBuf b, t;
    XELP_XB_INIT(b, (char*)args, len);
    if (XelpTokN(&b, n, &t) == XELP_S_OK) {
        int tl = (int)(t.p - t.s);
        if (tl >= bsz) tl = bsz - 1;
        memcpy(buf, t.s, tl);
        buf[tl] = '\0';
    } else {
        buf[0] = '\0';
    }
    return buf;
}

/* Extract token N as an integer. */
static int tokInt(const char* args, int len, int n) {
    char buf[16];
    tokStr(args, len, n, buf, sizeof(buf));
    return atoi(buf);
}

/* Count tokens. */
static int tokCount(const char* args, int len) {
    XelpBuf b;
    int n;
    XELP_XB_INIT(b, (char*)args, len);
    XelpNumToks(&b, &n);
    return n;
}

/* ------------------------------------------------------------------ */
/* Demo scripts (full targets only)                                    */
/* ------------------------------------------------------------------ */

#ifndef XELP_SMALL_TARGET
static const char DEMO_BLINK[] PROGMEM =
    "pinmode 13 out; setpin 13 1; delay 300; setpin 13 0; delay 300; "
    "setpin 13 1; delay 300; setpin 13 0; delay 300; "
    "setpin 13 1; delay 300; setpin 13 0";

static const char DEMO_SCAN[] PROGMEM =
    "pinmode 2 in; pinmode 3 in; pinmode 4 in; scanpins 2 4";

static const char DEMO_INFO[] PROGMEM =
    "echo --- board status ---; info; echo uptime:; millis";

/* Copy a PROGMEM string to a stack buffer and run it. */
static void runProgmem(const char* pgm) {
    char buf[128];
    strncpy_P(buf, pgm, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    Serial.println(F("Running:"));
    Serial.print(F("  "));
    Serial.println(buf);
    Serial.println();
    XelpParse(&cli, buf, (int)strlen(buf));
}
#endif /* XELP_SMALL_TARGET */

/* ------------------------------------------------------------------ */
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

XELPRESULT cmdHelp(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, XELP_BANNER_STR, 0);
    return XelpHelp(x);
}

XELPRESULT cmdBanner(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, XELP_BANNER_STR, 0);
    XelpOut(x, "Welcome to xelp CLI demo.\n", 0);
    XelpOut(x, "Type help to see commands. (also accepts ?)\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdEcho(XELP *x, const char* args, int len) {
    int n = tokCount(args, len);
    char buf[32];
    for (int i = 1; i < n; i++) {
        if (i > 1) XelpOut(x, " ", 1);
        tokStr(args, len, i, buf, sizeof(buf));
        XelpOut(x, buf, 0);
    }
    XelpOut(x, "\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdInfo(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    char buf[64];
#if defined(ARDUINO_BOARD)
    snprintf(buf, sizeof(buf), "Board:  %s\n", ARDUINO_BOARD);
#else
    snprintf(buf, sizeof(buf), "Board:  unknown\n");
#endif
    XelpOut(x, buf, 0);
    int mem = freeMemory();
    if (mem >= 0) {
        snprintf(buf, sizeof(buf), "Free:   %d bytes\n", mem);
        XelpOut(x, buf, 0);
    }
    snprintf(buf, sizeof(buf), "Uptime: %lu ms\n", millis());
    XelpOut(x, buf, 0);
    snprintf(buf, sizeof(buf), "xelp:   0x%08lX\n", (unsigned long)XELP_VERSION);
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

XELPRESULT cmdSetpin(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 3) {
        XelpOut(x, "usage: setpin <pin> <0|1>\n", 0);
        return XELP_E_ERR;
    }
    int pin = tokInt(args, len, 1);
    warnSerial(pin);
    digitalWrite(pin, tokInt(args, len, 2) ? HIGH : LOW);
    return XELP_S_OK;
}

XELPRESULT cmdGetpin(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 2) {
        XelpOut(x, "usage: getpin <pin>\n", 0);
        return XELP_E_ERR;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d\n", digitalRead(tokInt(args, len, 1)));
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

XELPRESULT cmdPinmode(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 3) {
        XelpOut(x, "usage: pinmode <pin> <in|out|pullup>\n", 0);
        return XELP_E_ERR;
    }
    int pin = tokInt(args, len, 1);
    char mode[8];
    tokStr(args, len, 2, mode, sizeof(mode));
    warnSerial(pin);
    if      (strcmp(mode, "out") == 0)    pinMode(pin, OUTPUT);
    else if (strcmp(mode, "in") == 0)     pinMode(pin, INPUT);
    else if (strcmp(mode, "pullup") == 0) pinMode(pin, INPUT_PULLUP);
    else XelpOut(x, "  expected: in, out, or pullup\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdSetpwm(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 3) {
        XelpOut(x, "usage: setpwm <pin> <0-255>\n", 0);
        return XELP_E_ERR;
    }
    int pin = tokInt(args, len, 1);
    warnSerial(pin);
    analogWrite(pin, tokInt(args, len, 2));
    return XELP_S_OK;
}

XELPRESULT cmdReadadc(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 2) {
        XelpOut(x, "usage: readadc <pin>\n", 0);
        return XELP_E_ERR;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d\n", analogRead(tokInt(args, len, 1)));
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

#ifndef XELP_SMALL_TARGET
XELPRESULT cmdTone(XELP *x, const char* args, int len) {
    int n = tokCount(args, len);
    if (n < 3) {
        XelpOut(x, "usage: tone <pin> <hz> [ms]\n", 0);
        return XELP_E_ERR;
    }
    int pin = tokInt(args, len, 1);
    unsigned int freq = (unsigned int)tokInt(args, len, 2);
    if (n >= 4) {
        char buf[16];
        tone(pin, freq, (unsigned long)atol(tokStr(args, len, 3, buf, sizeof(buf))));
    } else {
        tone(pin, freq);
    }
    return XELP_S_OK;
}

XELPRESULT cmdNotone(XELP *x, const char* args, int len) {
    (void)x;
    if (tokCount(args, len) < 2) return XELP_E_ERR;
    noTone(tokInt(args, len, 1));
    return XELP_S_OK;
}

XELPRESULT cmdPulsein(XELP *x, const char* args, int len) {
    int n = tokCount(args, len);
    if (n < 3) {
        XelpOut(x, "usage: pulsein <pin> <high|low> [timeout_us]\n", 0);
        return XELP_E_ERR;
    }
    int pin = tokInt(args, len, 1);
    char mode[8];
    tokStr(args, len, 2, mode, sizeof(mode));
    int level = (strcmp(mode, "high") == 0) ? HIGH : LOW;
    unsigned long timeout = 1000000UL;
    if (n >= 4) {
        char buf[16];
        timeout = (unsigned long)atol(tokStr(args, len, 3, buf, sizeof(buf)));
    }
    unsigned long dur = pulseIn(pin, level, timeout);
    char out[24];
    snprintf(out, sizeof(out), "%lu us\n", dur);
    XelpOut(x, out, 0);
    return XELP_S_OK;
}

XELPRESULT cmdMicros(XELP *x, const char* args, int len) {
    (void)args; (void)len;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu\n", micros());
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

XELPRESULT cmdScanpins(XELP *x, const char* args, int len) {
    int n = tokCount(args, len);
    int first = (n >= 2) ? tokInt(args, len, 1) : 0;
    int last  = (n >= 3) ? tokInt(args, len, 2) : NUM_DIGITAL_PINS - 1;
    char buf[24];
    XelpOut(x, "pin  value\n", 0);
    XelpOut(x, "---  -----\n", 0);
    for (int p = first; p <= last; p++) {
        snprintf(buf, sizeof(buf), "%3d  %5d\n", p, digitalRead(p));
        XelpOut(x, buf, 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdDemoBlink(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, "xelp can chain commands as scripts using semicolons.\n", 0);
    runProgmem(DEMO_BLINK);
    XelpOut(x, "Done.\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdDemoScan(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, "Set pins 2-4 as inputs, then scan them:\n", 0);
    runProgmem(DEMO_SCAN);
    return XELP_S_OK;
}

XELPRESULT cmdDemoInfo(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, "Chain multiple info commands in one line:\n", 0);
    runProgmem(DEMO_INFO);
    return XELP_S_OK;
}
#endif /* XELP_SMALL_TARGET */

XELPRESULT cmdDelay(XELP *x, const char* args, int len) {
    (void)x;
    if (tokCount(args, len) >= 2) {
        char buf[16];
        delay((unsigned long)atol(tokStr(args, len, 1, buf, sizeof(buf))));
    }
    return XELP_S_OK;
}

XELPRESULT cmdMillis(XELP *x, const char* args, int len) {
    (void)args; (void)len;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu\n", millis());
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

XELPRESULT cmdReboot(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, "Rebooting...\n", 0);
    delay(100);
#if defined(__AVR__)
    void (*resetFunc)(void) = 0;
    resetFunc();
#elif defined(ESP32) || defined(ESP8266)
    ESP.restart();
#else
    XelpOut(x, "  not supported on this board\n", 0);
#endif
    return XELP_S_OK;
}

/* Default handler for unrecognized commands. */
XELPRESULT cmdNotFound(XELP *x, const char* args, int len) {
    char buf[16];
    tokStr(args, len, 0, buf, sizeof(buf));
    XelpOut(x, buf, 0);
    XelpOut(x, ": unknown command\n", 0);
    return XELP_E_CMDNOTFOUND;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

#ifdef XELP_SMALL_TARGET
XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,       "help",        ""                                },
    { &cmdHelp,       "?",           ""                                },
    { &cmdBanner,     "banner",      ""                                },
    { &cmdEcho,       "echo",        "<args>"                          },
    { &cmdInfo,       "info",        ""                                },
    { &cmdSetpin,     "setpin",      "<pin> <0|1>"                     },
    { &cmdGetpin,     "getpin",      "<pin>"                           },
    { &cmdPinmode,    "pinmode",     "<pin> <in|out|pullup>"           },
    { &cmdSetpwm,     "setpwm",      "<pin> <0-255>"                  },
    { &cmdReadadc,    "readadc",     "<pin>"                           },
    { &cmdDelay,      "delay",       "<ms>"                            },
    { &cmdMillis,     "millis",      ""                                },
    { &cmdReboot,     "reboot",      ""                                },
    XELP_FUNC_ENTRY_LAST
};
#else
XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,       "help",        "show this help listing"          },
    { &cmdHelp,       "?",           "same as help"                    },
    { &cmdBanner,     "banner",      "print xelp banner"              },
    { &cmdEcho,       "echo",        "echo <args> -- print arguments"  },
    { &cmdInfo,       "info",        "board type, memory, uptime"      },
    { &cmdSetpin,     "setpin",      "setpin <pin> <0|1>"              },
    { &cmdGetpin,     "getpin",      "getpin <pin> -- digitalRead"     },
    { &cmdPinmode,    "pinmode",     "pinmode <pin> <in|out|pullup>"   },
    { &cmdSetpwm,     "setpwm",      "setpwm <pin> <0-255>"           },
    { &cmdReadadc,    "readadc",     "readadc <pin> -- analogRead"     },
    { &cmdTone,       "tone",        "tone <pin> <hz> [ms]"            },
    { &cmdNotone,     "notone",      "notone <pin> -- stop tone"       },
    { &cmdPulsein,    "pulsein",     "pulsein <pin> <high|low> [us]"   },
    { &cmdDelay,      "delay",       "delay <ms>"                      },
    { &cmdMillis,     "millis",      "uptime in milliseconds"          },
    { &cmdMicros,     "micros",      "uptime in microseconds"          },
    { &cmdScanpins,   "scanpins",    "scanpins [first] [last]"         },
    { &cmdDemoBlink,  "demo-blink3", "blink LED 3x (scripting demo)"  },
    { &cmdDemoScan,   "demo-scan",   "configure+scan (scripting demo)" },
    { &cmdDemoInfo,   "demo-info",   "echo+info+millis (script demo)"  },
    { &cmdReboot,     "reboot",      "software reset"                  },
    XELP_FUNC_ENTRY_LAST
};
#endif

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup() {
    Serial.begin(115200);
    while (!Serial) ;                   /* wait for USB serial (Leonardo) */
    pinMode(LED_BUILTIN, OUTPUT);

    XelpInit(&cli, "arduino-live-cli -- xelp hardware CLI demo\n");
    XELP_SET_FN_OUT(cli, &myOutput);
    XELP_SET_FN_CLI(cli, commands);
    XELP_SET_FN_DEF_CLI(cli, &cmdNotFound);

    Serial.println(F(XELP_BANNER_STR));
    Serial.println(F("Welcome to xelp CLI demo."));
    Serial.println(F("Type help to see commands. (also accepts ?)\n"));
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop() {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        XelpParseKey(&cli, c);
    }
}
