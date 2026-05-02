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

#define XELP_MAX_CLI_CMDS 24
#include "xelp.h"
#include "XelpArduino.h"

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

XelpCLI cli;

static char promptBuf[32] = "xelp>";

void myOutput(char c) { Serial.write(c); }

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

/* ------------------------------------------------------------------ */
/* Demo scripts                                                        */
/* ------------------------------------------------------------------ */

static const char DEMO_BLINK[] =
    "pinmode 13 out; setpin 13 1; delay 300; setpin 13 0; delay 300; "
    "setpin 13 1; delay 300; setpin 13 0; delay 300; "
    "setpin 13 1; delay 300; setpin 13 0";

static const char DEMO_SCAN[] =
    "pinmode 2 in; pinmode 3 in; pinmode 4 in; scanpins 2 4";

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup() {
    Serial.begin(115200);
    while (!Serial) ;                   /* wait for USB serial (Leonardo) */
    pinMode(LED_BUILTIN, OUTPUT);

    cli.begin("arduino-live-cli -- xelp hardware CLI demo\n", &myOutput);
    cli.setPrompt(promptBuf);

    cli.commands({

        /* ---- Help & info ------------------------------------------ */
        {"help", "show this help listing",
            [](XelpCLI& c, int, const char**) {
                c.help();
            }},

        {"?", "same as help",
            [](XelpCLI& c, int, const char**) {
                c.help();
            }},

        {"banner", "print xelp banner",
            [](XelpCLI& c, int, const char**) {
                c.print(XELP_BANNER_STR);
                c.print("Welcome to xelp CLI demo.\n");
                c.print("Type help to see commands. (also accepts ?)\n");
            }},

        {"echo", "echo <args> -- print arguments",
            [](XelpCLI& c, int argc, const char** argv) {
                for (int i = 1; i < argc; i++) {
                    if (i > 1) c.print(" ");
                    c.print(argv[i]);
                }
                c.print("\n");
            }},

        {"info", "print board type, memory, uptime",
            [](XelpCLI& c, int, const char**) {
                char buf[64];
#if defined(ARDUINO_BOARD)
                snprintf(buf, sizeof(buf), "Board:  %s\n", ARDUINO_BOARD);
#else
                snprintf(buf, sizeof(buf), "Board:  unknown\n");
#endif
                c.print(buf);
                int mem = freeMemory();
                if (mem >= 0) {
                    snprintf(buf, sizeof(buf), "Free:   %d bytes\n", mem);
                    c.print(buf);
                }
                snprintf(buf, sizeof(buf), "Uptime: %lu ms\n", millis());
                c.print(buf);
                snprintf(buf, sizeof(buf), "xelp:   0x%08lX\n",
                         (unsigned long)XELP_VERSION);
                c.print(buf);
            }},

        /* ---- Digital I/O ------------------------------------------ */
        {"setpin", "setpin <pin> <0|1> -- digitalWrite",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 3) { c.print("usage: setpin <pin> <0|1>\n"); return; }
                int pin = atoi(argv[1]);
                warnSerial(pin);
                digitalWrite(pin, atoi(argv[2]) ? HIGH : LOW);
            }},

        {"getpin", "getpin <pin> -- digitalRead",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 2) { c.print("usage: getpin <pin>\n"); return; }
                char buf[8];
                snprintf(buf, sizeof(buf), "%d\n", digitalRead(atoi(argv[1])));
                c.print(buf);
            }},

        {"pinmode", "pinmode <pin> <in|out|pullup>",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 3) { c.print("usage: pinmode <pin> <in|out|pullup>\n"); return; }
                int pin = atoi(argv[1]);
                warnSerial(pin);
                if      (strcmp(argv[2], "out") == 0)    pinMode(pin, OUTPUT);
                else if (strcmp(argv[2], "in") == 0)     pinMode(pin, INPUT);
                else if (strcmp(argv[2], "pullup") == 0) pinMode(pin, INPUT_PULLUP);
                else c.print("  expected: in, out, or pullup\n");
            }},

        /* ---- Analog ----------------------------------------------- */
        {"setpwm", "setpwm <pin> <0-255> -- analogWrite",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 3) { c.print("usage: setpwm <pin> <0-255>\n"); return; }
                int pin = atoi(argv[1]);
                warnSerial(pin);
                analogWrite(pin, atoi(argv[2]));
            }},

        {"readadc", "readadc <pin> -- analogRead (0-1023)",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 2) { c.print("usage: readadc <pin>\n"); return; }
                char buf[8];
                snprintf(buf, sizeof(buf), "%d\n", analogRead(atoi(argv[1])));
                c.print(buf);
            }},

        /* ---- Tone ------------------------------------------------- */
        {"tone", "tone <pin> <hz> [ms] -- play frequency",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 3) { c.print("usage: tone <pin> <hz> [ms]\n"); return; }
                int pin  = atoi(argv[1]);
                unsigned int freq = (unsigned int)atoi(argv[2]);
                if (argc >= 4)
                    tone(pin, freq, (unsigned long)atol(argv[3]));
                else
                    tone(pin, freq);
            }},

        {"notone", "notone <pin> -- stop tone",
            [](XelpCLI&, int argc, const char** argv) {
                if (argc < 2) return;
                noTone(atoi(argv[1]));
            }},

        /* ---- Pulse measurement ------------------------------------ */
        {"pulsein", "pulsein <pin> <high|low> [timeout_us]",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 3) { c.print("usage: pulsein <pin> <high|low> [timeout_us]\n"); return; }
                int pin = atoi(argv[1]);
                int level = (strcmp(argv[2], "high") == 0) ? HIGH : LOW;
                unsigned long timeout = (argc >= 4)
                    ? (unsigned long)atol(argv[3]) : 1000000UL;
                unsigned long dur = pulseIn(pin, level, timeout);
                char buf[24];
                snprintf(buf, sizeof(buf), "%lu us\n", dur);
                c.print(buf);
            }},

        /* ---- Timing ----------------------------------------------- */
        {"delay", "delay <ms> -- pause execution",
            [](XelpCLI&, int argc, const char** argv) {
                if (argc >= 2) delay((unsigned long)atol(argv[1]));
            }},

        {"millis", "print uptime in milliseconds",
            [](XelpCLI& c, int, const char**) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%lu\n", millis());
                c.print(buf);
            }},

        {"micros", "print uptime in microseconds",
            [](XelpCLI& c, int, const char**) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%lu\n", micros());
                c.print(buf);
            }},

        /* ---- Pin scan --------------------------------------------- */
        {"scanpins", "scanpins [first] [last] -- read digital pins",
            [](XelpCLI& c, int argc, const char** argv) {
                int first = (argc >= 2) ? atoi(argv[1]) : 0;
                int last  = (argc >= 3) ? atoi(argv[2]) : NUM_DIGITAL_PINS - 1;
                char buf[24];
                c.print("pin  value\n");
                c.print("---  -----\n");
                for (int p = first; p <= last; p++) {
                    snprintf(buf, sizeof(buf), "%3d  %5d\n", p, digitalRead(p));
                    c.print(buf);
                }
            }},

        /* ---- Prompt ----------------------------------------------- */
        {"setprompt", "setprompt <text> -- change CLI prompt",
            [](XelpCLI& c, int argc, const char** argv) {
                if (argc < 2) { c.print("usage: setprompt <text>\n"); return; }
                strncpy(promptBuf, argv[1], sizeof(promptBuf) - 1);
                promptBuf[sizeof(promptBuf) - 1] = '\0';
                c.setPrompt(promptBuf);
            }},

        /* ---- Demos ------------------------------------------------ */
        {"demo-blink3", "blink LED 3x (scripting demo)",
            [](XelpCLI& c, int, const char**) {
                c.print("xelp can chain commands as scripts using semicolons.\n");
                c.print("Running:\n  ");
                c.print(DEMO_BLINK);
                c.print("\n\n");
                c.run(DEMO_BLINK);
                c.print("Done.\n");
            }},

        {"demo-scan", "configure + scan pins (scripting demo)",
            [](XelpCLI& c, int, const char**) {
                c.print("Set pins 2-4 as inputs, then scan them:\n  ");
                c.print(DEMO_SCAN);
                c.print("\n\n");
                c.run(DEMO_SCAN);
            }},

        {"demo-info", "echo + info + millis (scripting demo)",
            [](XelpCLI& c, int, const char**) {
                static const char script[] =
                    "echo --- board status ---; info; echo uptime:; millis";
                c.print("Chain multiple info commands in one line:\n  ");
                c.print(script);
                c.print("\n\n");
                c.run(script);
            }},

        /* ---- Reboot ----------------------------------------------- */
        {"reboot", "software reset",
            [](XelpCLI& c, int, const char**) {
                c.print("Rebooting...\n");
                delay(100);
#if defined(__AVR__)
                void (*resetFunc)(void) = 0;
                resetFunc();
#elif defined(ESP32) || defined(ESP8266)
                ESP.restart();
#else
                c.print("  not supported on this board\n");
#endif
            }},
    });

    Serial.println(F(XELP_BANNER_STR));
    Serial.println(F("Welcome to xelp CLI demo."));
    Serial.println(F("Type help to see commands. (also accepts ?)\n"));
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop() {
    cli.poll(Serial);
}
