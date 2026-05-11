/*
 * esp32-ble-cli.ino -- ESP32 dual-CLI demo: Serial + BLE.
 *
 * Two independent xelp instances sharing one command table: one on USB
 * Serial, one on BLE (Nordic UART Service).  Demonstrates zero-dynamic-
 * memory multi-instance CLI over two different transports.
 *
 * Compatible with any ESP32 variant that has BLE:
 *   ESP32, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2
 *
 * Commands (identical on both Serial and BLE):
 *   help / ?      list commands
 *   banner        print welcome banner
 *   echo <args>   print arguments
 *   info          app version, board type, uptime, xelp version
 *   led <0|1>     built-in LED on/off
 *   rgb <r> <g> <b>  set RGB LED color (boards with NeoPixel)
 *   setpin        setpin <pin> <0|1>
 *   getpin        getpin <pin> -- digitalRead
 *   pinmode       pinmode <pin> <in|out|pullup>
 *   setpwm        setpwm <pin> <0-255>
 *   readadc       readadc <pin> -- analogRead
 *   delay         delay <ms>
 *   millis        uptime in milliseconds
 *   status        serial, BLE, LED type, uptime
 *   sendmsg       sendmsg <serial|ble> <text> -- cross-instance msg
 *   reboot        software reset (ESP.restart)
 *
 * Open Serial Monitor at 115200 baud, or connect from the companion
 * web app (examples/esp32-ble-cli/web/index.html) over Web Bluetooth.
 *
 * Requires:
 *   - ESP32 Arduino core (Board Manager: esp32 by Espressif)
 *   - NimBLE-Arduino (Library Manager: search "NimBLE-Arduino" by h2zero)
 *
 * IMPORTANT -- Partition scheme:
 *   BLE may exceed the default 1.3 MB partition on some ESP32 variants.
 *   In Arduino IDE select Tools > Partition Scheme >
 *   "Huge APP (3MB No OTA/1MB SPIFFS)".
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee <deftio [at] deftio [dot] com>
 * BSD-2-Clause -- see LICENSE.txt
 */

#include <NimBLEDevice.h>

#include "xelp.h"
#include "XelpArduino.h"

#define APP_VERSION "0.1.0"

/* ------------------------------------------------------------------ */
/* Board defaults                                                      */
/* ------------------------------------------------------------------ */

/* Most ESP32 boards define LED_BUILTIN for a simple GPIO LED.
   Some (e.g. Unexpected Maker ProS3) use an addressable RGB NeoPixel
   via RGB_BUILTIN + rgbLedWrite() instead.  We support both.
   On ProS3: RGB data = GPIO 18, RGB power (LDO2) = GPIO 17.
   RGB_PWR must be driven HIGH before rgbLedWrite() will produce output. */
#ifdef RGB_BUILTIN
#define HAS_RGB_LED 1
#else
#define HAS_RGB_LED 0
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#endif

/* ------------------------------------------------------------------ */
/* Nordic UART Service UUIDs                                           */
/* ------------------------------------------------------------------ */

#define NUS_SERVICE_UUID "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_RX_UUID      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_TX_UUID      "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

static XelpCLI       cliSerial;
static XelpCLI       cliBle;

static NimBLEServer*         pServer  = nullptr;
static NimBLECharacteristic* pTxChar  = nullptr;
static bool                  bleConnected = false;
static uint16_t              bleConnId = 0;
static bool                  bleSendBanner = false;
static bool                  serialBannerSent = false;

/* Cross-instance message queue (sendmsg).  Messages are buffered here and
   delivered from loop() to avoid threading issues when the BLE callback
   task and the Arduino loop task both write to Serial simultaneously. */
static char  crossMsg[128];
static int   crossMsgLen = 0;
static XELP* crossMsgDest = nullptr;

/* ------------------------------------------------------------------ */
/* Output functions                                                    */
/* ------------------------------------------------------------------ */

static void serialOut(char c) {
    if (c == '\n') Serial.write('\r');  /* raw terminals need \r\n */
    Serial.write(c);
}

/* Buffered BLE output -- accumulate into a buffer, then drip-feed one
   small notification per loop() iteration.  This avoids overwhelming the
   BLE TX queue (which is only ~5 deep in NimBLE).  One chunk per connection
   event (~15-30ms) ensures nothing is silently dropped. */

static uint8_t bleTxBuf[1536];
static int     bleTxPos = 0;

/* Send at most one chunk (up to 20 bytes) from the front of bleTxBuf.
   Returns true if there's more data remaining. */
static bool bleTxDrip() {
    if (bleTxPos <= 0 || !bleConnected || !pTxChar) return false;

    int chunk = bleTxPos;
    if (chunk > 20) chunk = 20;

    pTxChar->setValue(bleTxBuf, chunk);
    if (pTxChar->notify()) {
        /* shift remaining data to front */
        bleTxPos -= chunk;
        if (bleTxPos > 0)
            memmove(bleTxBuf, bleTxBuf + chunk, bleTxPos);
    }
    /* else: TX queue full, will retry next loop iteration */
    return (bleTxPos > 0);
}

/* Flush entire buffer (used for banner on connect where we want it all
   sent before returning).  Paces at 1 chunk per 15ms. */
static void bleTxFlush() {
    while (bleTxPos > 0 && bleConnected) {
        bleTxDrip();
        if (bleTxPos > 0) delay(15);
    }
}

static void bleOut(char c) {
    if (!bleConnected) return;
    bleTxBuf[bleTxPos++] = (uint8_t)c;
    if (bleTxPos >= (int)sizeof(bleTxBuf))
        bleTxFlush();
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

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
    XelpOut(x, "ESP32 BLE CLI demo.\n", 0);
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
    snprintf(buf, sizeof(buf), "App:    esp32-ble-cli v%s\n", APP_VERSION);
    XelpOut(x, buf, 0);
#if defined(ARDUINO_BOARD)
    snprintf(buf, sizeof(buf), "Board:  %s\n", ARDUINO_BOARD);
#else
    snprintf(buf, sizeof(buf), "Board:  unknown\n");
#endif
    XelpOut(x, buf, 0);
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

XELPRESULT cmdLed(XELP *x, const char* args, int len) {
    if (tokCount(args, len) < 2) {
        XelpOut(x, "usage: led <0|1>\n", 0);
        return XELP_E_ERR;
    }
    int val = tokInt(args, len, 1);
#if HAS_RGB_LED
    rgbLedWrite(RGB_BUILTIN, val ? 32 : 0, val ? 32 : 0, val ? 32 : 0);
#else
    digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
#endif
    XelpOut(x, val ? "LED ON\n" : "LED OFF\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdRgb(XELP *x, const char* args, int len) {
#if HAS_RGB_LED
    int n = tokCount(args, len);
    if (n < 4) {
        XelpOut(x, "usage: rgb <r> <g> <b>  (0-255 each)\n", 0);
        return XELP_E_ERR;
    }
    int r = tokInt(args, len, 1);
    int g = tokInt(args, len, 2);
    int b = tokInt(args, len, 3);
    rgbLedWrite(RGB_BUILTIN, r, g, b);
    char buf[32];
    snprintf(buf, sizeof(buf), "RGB(%d,%d,%d)\n", r, g, b);
    XelpOut(x, buf, 0);
#else
    (void)args; (void)len;
    XelpOut(x, "no RGB LED on this board\n", 0);
#endif
    return XELP_S_OK;
}

XELPRESULT cmdStatus(XELP *x, const char* args, int len) {
    (void)args; (void)len;
    char buf[64];
    XelpOut(x, "Serial: connected\n", 0);
    XelpOut(x, bleConnected ? "BLE:    connected\n" : "BLE:    advertising\n", 0);
#if HAS_RGB_LED
    XelpOut(x, "LED:    RGB (NeoPixel)\n", 0);
#else
    snprintf(buf, sizeof(buf), "LED:    GPIO %d\n", LED_BUILTIN);
    XelpOut(x, buf, 0);
#endif
    snprintf(buf, sizeof(buf), "Uptime: %lu ms\n", millis());
    XelpOut(x, buf, 0);
    return XELP_S_OK;
}

/* Send a message to the other xelp instance.
   Usage: sendmsg serial|ble <text...>
   All tokens after the target are concatenated and queued for delivery
   in loop().  This avoids threading issues when BLE callbacks and the
   Arduino loop task both write to Serial simultaneously. */
XELPRESULT cmdSendmsg(XELP *x, const char* args, int len) {
    int n = tokCount(args, len);
    if (n < 3) {
        XelpOut(x, "usage: sendmsg <serial|ble> <message...>\n", 0);
        return XELP_E_ERR;
    }

    /* Determine target instance */
    char target[8];
    tokStr(args, len, 1, target, sizeof(target));
    XELP *dest = nullptr;
    if (strcmp(target, "serial") == 0 || strcmp(target, "ser") == 0)
        dest = cliSerial.raw();
    else if (strcmp(target, "ble") == 0)
        dest = cliBle.raw();
    else {
        XelpOut(x, "  target must be 'serial' or 'ble'\n", 0);
        return XELP_E_ERR;
    }

    /* Build message into crossMsg buffer for loop() to deliver */
    int pos = 0;
    pos += snprintf(crossMsg + pos, sizeof(crossMsg) - pos, "[msg] ");
    char buf[32];
    for (int i = 2; i < n; i++) {
        if (i > 2 && pos < (int)sizeof(crossMsg) - 1)
            crossMsg[pos++] = ' ';
        tokStr(args, len, i, buf, sizeof(buf));
        pos += snprintf(crossMsg + pos, sizeof(crossMsg) - pos, "%s", buf);
    }
    if (pos < (int)sizeof(crossMsg) - 1)
        crossMsg[pos++] = '\n';
    crossMsg[pos] = '\0';
    crossMsgLen = pos;
    crossMsgDest = dest;

    XelpOut(x, "sent\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdReboot(XELP *x, const char* a, int l) {
    (void)a; (void)l;
    XelpOut(x, "Rebooting...\n", 0);
    delay(100);
    ESP.restart();
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
/* Command table (shared by both instances)                            */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,     "help",    "show this help listing"           },
    { &cmdHelp,     "?",       "same as help"                     },
    { &cmdBanner,   "banner",  "print xelp banner"                },
    { &cmdEcho,     "echo",    "echo <args> -- print arguments"   },
    { &cmdInfo,     "info",    "app version, board, uptime"         },
    { &cmdLed,      "led",     "led <0|1> -- built-in LED"        },
    { &cmdRgb,      "rgb",     "rgb <r> <g> <b> -- set RGB LED"   },
    { &cmdSetpin,   "setpin",  "setpin <pin> <0|1>"               },
    { &cmdGetpin,   "getpin",  "getpin <pin> -- digitalRead"      },
    { &cmdPinmode,  "pinmode", "pinmode <pin> <in|out|pullup>"    },
    { &cmdSetpwm,   "setpwm",  "setpwm <pin> <0-255>"            },
    { &cmdReadadc,  "readadc", "readadc <pin> -- analogRead"      },
    { &cmdDelay,    "delay",   "delay <ms>"                       },
    { &cmdMillis,   "millis",  "uptime in milliseconds"           },
    { &cmdStatus,   "status",  "serial/BLE/LED/uptime status"     },
    { &cmdSendmsg,  "sendmsg", "sendmsg <serial|ble> <text...>"   },
    { &cmdReboot,   "reboot",  "software reset"                   },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* BLE callbacks                                                       */
/* ------------------------------------------------------------------ */

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *s, NimBLEConnInfo &connInfo) override {
        (void)s;
        bleConnected = true;
        bleConnId = connInfo.getConnHandle();
        bleSendBanner = true;
        Serial.println("[BLE] client connected");
    }
    void onDisconnect(NimBLEServer *s, NimBLEConnInfo &connInfo, int reason) override {
        (void)s; (void)connInfo; (void)reason;
        bleConnected = false;
        bleTxPos = 0;
        Serial.println("[BLE] client disconnected");
        NimBLEDevice::startAdvertising();
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *c, NimBLEConnInfo &connInfo) override {
        (void)connInfo;
        std::string val = c->getValue();
        for (size_t i = 0; i < val.length(); i++) {
            XelpParseKey(cliBle.raw(), val[i]);
        }
        /* Don't flush here — notify() from within the NimBLE callback task
           is unreliable and causes dropped notifications.  loop() handles it. */
    }
};

/* ------------------------------------------------------------------ */
/* BLE setup                                                           */
/* ------------------------------------------------------------------ */

static void bleSetup(void)
{
    NimBLEDevice::init("xelp-ble");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(NUS_SERVICE_UUID);

    /* TX characteristic: xelp output -> phone (notify) */
    pTxChar = pService->createCharacteristic(
        NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

    /* RX characteristic: phone -> xelp input (write) */
    NimBLECharacteristic *pRxChar = pService->createCharacteristic(
        NUS_RX_UUID, NIMBLE_PROPERTY::WRITE);
    pRxChar->setCallbacks(new RxCallbacks());

    pService->start();

    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE_UUID);
    NimBLEDevice::startAdvertising();

    Serial.println("[BLE] advertising as \"xelp-ble\"");
}

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 3000) ;  /* wait for USB CDC */
    delay(500);  /* let host terminal attach before printing banner */

#if HAS_RGB_LED
    /* On boards like ProS3/TinyS3/FeatherS3, the NeoPixel is powered via
       LDO2 (GPIO 17).  Must enable power before rgbLedWrite() works.
       RGB_PWR is declared as static const in pins_arduino.h (not a macro),
       so we reference the variable directly. */
    pinMode(RGB_PWR, OUTPUT);
    digitalWrite(RGB_PWR, HIGH);
    delay(10);  /* let LDO stabilize */
#else
    pinMode(LED_BUILTIN, OUTPUT);
#endif

    /* Serial CLI */
    cliSerial.begin("ESP32 BLE CLI -- xelp dual-instance demo\n", &serialOut);
    cliSerial.setCommands(commands);
    cliSerial.setDefaultCommandHandler(&cmdNotFound);

    /* BLE CLI */
    cliBle.begin("ESP32 BLE CLI (BLE)\n", &bleOut);
    cliBle.setCommands(commands);
    cliBle.setDefaultCommandHandler(&cmdNotFound);

    /* Start BLE */
    bleSetup();

    /* Defer serial banner to first keypress -- USB CDC on ESP32-S3 may not
       have a host terminal attached yet, so printing at boot gets lost. */
    Serial.println("\n[press Enter for CLI]");
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop()
{
    /* Show banner on first serial input (avoids USB CDC boot truncation). */
    if (!serialBannerSent && Serial.available()) {
        serialBannerSent = true;
        cliSerial.run("help");
        cliSerial.parseKey('\r');
    }

    cliSerial.poll(Serial);

    /* Deliver cross-instance messages (from sendmsg) safely from loop(). */
    if (crossMsgDest && crossMsgLen > 0) {
        XelpOut(crossMsgDest, crossMsg, crossMsgLen);
        crossMsgDest = nullptr;
        crossMsgLen = 0;
    }

    /* Send banner after BLE connect -- deferred to loop() so the client
       has time to subscribe to TX notifications first. */
    if (bleSendBanner && bleConnected) {
        bleSendBanner = false;
        delay(500);  /* let client finish GATT setup + subscribe */
        cliBle.run("help");
        cliBle.parseKey('\r');  /* show prompt */
        bleTxFlush();
    }

    /* Drip-feed BLE output: send one 20-byte chunk per loop iteration.
       This paces notifications at ~1 per connection interval so the BLE
       stack never overflows its TX queue. */
    if (bleTxPos > 0) {
        bleTxDrip();
        delay(15);  /* ~1 connection interval */
    }
}
