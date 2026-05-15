/*
 * main.c -- Raspberry Pi Pico CLI example using xelp (pure C, Pico SDK).
 *
 * Demonstrates xelp on the RP2040/RP2350 with USB CDC serial.
 * Commands: help, led, pin, set, get, adc, pwm
 * Key commands: ? (help), l (toggle LED)
 *
 * Build:
 *   mkdir build && cd build
 *   cmake -DPICO_BOARD=pico ..   # or pico_w, pico2, pico2_w
 *   make
 *
 * Flash the resulting .uf2 file and open a serial terminal at 115200.
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee <deftio [at] deftio [dot] com>
 * BSD-2-Clause -- see xelp.h for full license text.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

/* Pico W uses the CYW43 driver for the on-board LED. */
#if defined(CYW43_WL_GPIO_LED_PIN)
#include "pico/cyw43_arch.h"
#define LED_PIN CYW43_WL_GPIO_LED_PIN
#define LED_IS_CYW43 1
#else
#define LED_PIN PICO_DEFAULT_LED_PIN
#define LED_IS_CYW43 0
#endif

#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform abstraction                                                */
/* ------------------------------------------------------------------ */

static void uart_putc_fn(char c) { putchar(c); }

static void uart_bksp(void)
{
    putchar('\b');
    putchar(' ');
    putchar('\b');
}

/* ------------------------------------------------------------------ */
/* LED helpers (Pico vs Pico W)                                        */
/* ------------------------------------------------------------------ */

static int g_led_state = 0;

static void led_set(int on)
{
    g_led_state = on ? 1 : 0;
#if LED_IS_CYW43
    cyw43_arch_gpio_put(LED_PIN, g_led_state);
#else
    gpio_put(LED_PIN, g_led_state);
#endif
}

static void led_toggle(void)
{
    led_set(!g_led_state);
}

/* ------------------------------------------------------------------ */
/* KEY mode commands                                                   */
/* ------------------------------------------------------------------ */

static XELPRESULT key_help(XELP *ths, XELPKEYCODE c)
{
    (void)c;
    return XelpHelp(ths);
}

static XELPRESULT key_led_toggle(XELP *ths, XELPKEYCODE c)
{
    (void)c;
    led_toggle();
    XelpOut(ths, g_led_state ? "LED ON\n" : "LED OFF\n", 0);
    return XELP_S_OK;
}

XELPKeyFuncMapEntry key_commands[] = {
    { &key_help,       '?', "show help"   },
    { &key_led_toggle, 'l', "toggle LED"  },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* CLI mode commands                                                   */
/* ------------------------------------------------------------------ */

static XELPRESULT cmd_help(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

static XELPRESULT cmd_led(XELP *ths, int argc, const char **argv)
{
    if (argc > 1) {
        int val = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
        led_set(val);
        XelpOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    } else {
        XelpOut(ths, "usage: led <0|1>\n", 0);
    }
    return XELP_S_OK;
}

static XELPRESULT cmd_pin(XELP *ths, int argc, const char **argv)
{
    int pin;

    if (argc < 3) goto usage;
    pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));

    gpio_init(pin);
    if (argv[2][0] == 'o') {
        gpio_set_dir(pin, GPIO_OUT);
        XelpOut(ths, "output\n", 0);
    } else {
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        XelpOut(ths, "input (pull-up)\n", 0);
    }
    return XELP_S_OK;

usage:
    XelpOut(ths, "usage: pin <n> <in|out>\n", 0);
    return XELP_E_ERR;
}

static XELPRESULT cmd_set(XELP *ths, int argc, const char **argv)
{
    int pin, val;

    if (argc < 3) goto usage;
    pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
    val = XelpStr2Int(argv[2], XelpStrLen(argv[2]));

    gpio_put(pin, val ? 1 : 0);
    XelpOut(ths, val ? "HIGH\n" : "LOW\n", 0);
    return XELP_S_OK;

usage:
    XelpOut(ths, "usage: set <pin> <0|1>\n", 0);
    return XELP_E_ERR;
}

static XELPRESULT cmd_get(XELP *ths, int argc, const char **argv)
{
    int pin, val;

    if (argc < 2) {
        XelpOut(ths, "usage: get <pin>\n", 0);
        return XELP_E_ERR;
    }
    pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
    val = gpio_get(pin);
    XelpOut(ths, val ? "HIGH\n" : "LOW\n", 0);
    ths->mR[1] = val;
    return XELP_S_OK;
}

static XELPRESULT cmd_adc(XELP *ths, int argc, const char **argv)
{
    int ch;

    if (argc < 2) {
        XelpOut(ths, "usage: adc <0-3>\n", 0);
        return XELP_E_ERR;
    }
    ch = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
    if (ch < 0 || ch > 3) {
        XelpOut(ths, "channel 0-3 only\n", 0);
        return XELP_E_ERR;
    }

    adc_select_input(ch);
    uint16_t raw = adc_read();
    char buf[32];
    snprintf(buf, sizeof(buf), "ADC%d: %u\n", ch, raw);
    XelpOut(ths, buf, 0);
    ths->mR[1] = raw;
    return XELP_S_OK;
}

static XELPRESULT cmd_pwm(XELP *ths, int argc, const char **argv)
{
    int pin, duty;

    if (argc < 3) goto usage;
    pin = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
    duty = XelpStr2Int(argv[2], XelpStrLen(argv[2]));

    {
        gpio_set_function(pin, GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(pin);
        uint channel = pwm_gpio_to_channel(pin);
        pwm_set_wrap(slice, 255);
        pwm_set_chan_level(slice, channel, duty & 0xFF);
        pwm_set_enabled(slice, true);
        XelpOut(ths, "OK\n", 0);
    }
    return XELP_S_OK;

usage:
    XelpOut(ths, "usage: pwm <pin> <0-255>\n", 0);
    return XELP_E_ERR;
}

XELPCLIFuncMapEntry cli_commands[] = {
    { &cmd_help, "help", "show help"                },
    { &cmd_led,  "led",  "led <0|1> -- set LED"     },
    { &cmd_pin,  "pin",  "pin <n> <in|out> -- init" },
    { &cmd_set,  "set",  "set <pin> <0|1>"          },
    { &cmd_get,  "get",  "get <pin> -- read level"  },
    { &cmd_adc,  "adc",  "adc <0-3> -- read ADC"    },
    { &cmd_pwm,  "pwm",  "pwm <pin> <0-255>"        },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

XELP cli;

int main(void)
{
    stdio_init_all();

#if LED_IS_CYW43
    cyw43_arch_init();
#else
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif

    /* ADC init for channels 0-3 (GP26-GP29) */
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_gpio_init(28);

    XelpInit(&cli, "Pico CLI (xelp)\n");

    XELP_SET_FN_OUT(cli, &uart_putc_fn);
    XELP_SET_FN_BKSP(cli, &uart_bksp);
    XELP_SET_FN_KEY(cli, key_commands);
    XELP_SET_FN_CLI(cli, cli_commands);

    XelpOut(&cli, XELP_BANNER_STR, 0);
    XelpHelp(&cli);
    XelpParseKey(&cli, '\n');  /* show initial prompt */

    for (;;) {
        int c = getchar_timeout_us(0);
        if (c >= 0) {
            XelpParseKey(&cli, (char)c);
        }
    }
}
