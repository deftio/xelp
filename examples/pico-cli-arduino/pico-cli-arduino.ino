/*
 * pico-cli-arduino.ino -- Raspberry Pi Pico Easy API example.
 *
 * Demonstrates the C++ Easy API: commands({...}) with inline lambdas.
 * No static command tables, no raw XELP* pointers, no function-pointer
 * arrays.
 *
 * Board: Raspberry Pi Pico / Pico W (Arduino-Pico core by Earle Philhower)
 * Install: https://github.com/earlephilhower/arduino-pico
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee <deftio [at] deftio [dot] com>
 * BSD-2-Clause -- see xelp.h for full license text.
 */

#include "xelp.h"
#include "XelpArduino.h"

XelpCLI cli;

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 3000) ;

    pinMode(LED_BUILTIN, OUTPUT);

    cli.begin("Pico CLI Demo (Easy API)\n", nullptr);
    cli.output(Serial);
    cli.prompt("pico>");

    cli.commands({
        {"help", "show help", [](XelpCLI& c, int, const char**) {
            c.help();
        }},
        {"led", "led <0|1>", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 2) { c.print("usage: led <0|1>\n"); return; }
            int v = argv[1][0] - '0';
            digitalWrite(LED_BUILTIN, v ? HIGH : LOW);
            c.print(v ? "LED ON\n" : "LED OFF\n");
        }},
        {"adc", "adc <pin> -- read analog", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 2) { c.print("usage: adc <pin>\n"); return; }
            int pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
            int val = analogRead(pin);
            char buf[32];
            snprintf(buf, sizeof(buf), "A%d: %d\n", pin, val);
            c.print(buf);
        }},
        {"pwm", "pwm <pin> <0-255>", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 3) { c.print("usage: pwm <pin> <duty>\n"); return; }
            int pin  = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
            int duty = XelpStr2Int(argv[2], XelpStrLen(argv[2]));
            analogWrite(pin, duty);
            c.print("OK\n");
        }},
        {"pin", "pin <n> <in|out>", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 3) { c.print("usage: pin <n> <in|out>\n"); return; }
            int pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
            if (argv[2][0] == 'o') {
                pinMode(pin, OUTPUT);
                c.print("output\n");
            } else {
                pinMode(pin, INPUT_PULLUP);
                c.print("input (pull-up)\n");
            }
        }},
        {"set", "set <pin> <0|1>", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 3) { c.print("usage: set <pin> <0|1>\n"); return; }
            int pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
            int val = XelpStr2Int(argv[2], XelpStrLen(argv[2]));
            digitalWrite(pin, val ? HIGH : LOW);
            c.print(val ? "HIGH\n" : "LOW\n");
        }},
        {"get", "get <pin> -- read level", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 2) { c.print("usage: get <pin>\n"); return; }
            int pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
            int val = digitalRead(pin);
            c.print(val ? "HIGH\n" : "LOW\n");
        }},
    });

    cli.keyCommands({
        {'?', "show help", [](XelpCLI& c, XELPKEYCODE) {
            c.help();
        }},
        {'l', "toggle LED", [](XelpCLI& c, XELPKEYCODE) {
            static bool on = false;
            on = !on;
            digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
            c.print(on ? "LED ON\n" : "LED OFF\n");
        }},
    });

    cli.help();
}

void loop()
{
    cli.poll(Serial);
}
