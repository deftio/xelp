/*
 * esp32c6-wifi.ino -- XIAO ESP32-C6 dual-CLI demo.
 *
 * Two independent xelp instances: one on USB Serial, one on BLE (Nordic
 * UART Service).  Both share the same command table:
 *
 *   ssid <name>      set WiFi SSID (saved to NVS)
 *   wifipass <pw>    set WiFi password (saved to NVS)
 *   connect          connect to WiFi
 *   disconnect       disconnect from WiFi
 *   status           show WiFi / BLE / IP info
 *   led <0|1>        toggle the on-board LED
 *   adc              read A0 voltage (16-sample average, 1:2 divider)
 *   help             list commands
 *
 * Hardware: Seeed XIAO ESP32-C6
 *   - LED_BUILTIN (GPIO15)
 *   - A0 analog input (with external 1:2 voltage divider for battery)
 *
 * Requires:
 *   - ESP32 Arduino core (WiFi, Preferences)
 *   - NimBLE-Arduino (Library Manager: search "NimBLE-Arduino" by h2zero)
 *
 * IMPORTANT -- Partition scheme:
 *   WiFi + BLE together exceed the default 1.3 MB partition.  In Arduino
 *   IDE select Tools > Partition Scheme > "Huge APP (3MB No OTA/1MB SPIFFS)".
 *   This is an ESP-IDF / Arduino core limitation, not xelp.  xelp itself
 *   adds ~4-5 KB.  The WiFi + NimBLE stacks account for the rest.
 *
 * Open Serial Monitor at 115200 baud, or connect from the companion
 * web app (examples/esp32c6-wifi/web/index.html) over Web Bluetooth.
 */

#include <WiFi.h>
#include <Preferences.h>
#include <NimBLEDevice.h>

#include "xelp.h"
#include "XelpArduino.h"

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

static Preferences   prefs;
static char          gSsid[33]  = "";
static char          gPass[65]  = "";

static NimBLEServer*         pServer  = nullptr;
static NimBLECharacteristic* pTxChar  = nullptr;
static bool                  bleConnected = false;

/* ------------------------------------------------------------------ */
/* Output functions                                                    */
/* ------------------------------------------------------------------ */

static void serialOut(char c) { Serial.write(c); }

static void bleOut(char c)
{
    if (!bleConnected || !pTxChar) return;
    uint8_t byte = (uint8_t)c;
    pTxChar->setValue(&byte, 1);
    pTxChar->notify();
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void tokCopy(char *dest, int maxlen, const char *s, const char *e)
{
    int len = e - s;
    if (len >= maxlen) len = maxlen - 1;
    memcpy(dest, s, len);
    dest[len] = '\0';
}

static void nvsSave(void)
{
    prefs.begin("xelp", false);
    prefs.putString("ssid", gSsid);
    prefs.putString("pass", gPass);
    prefs.end();
}

static void nvsLoad(void)
{
    prefs.begin("xelp", true);
    String s = prefs.getString("ssid", "");
    String p = prefs.getString("pass", "");
    prefs.end();
    s.toCharArray(gSsid, sizeof(gSsid));
    p.toCharArray(gPass, sizeof(gPass));
}

/* ------------------------------------------------------------------ */
/* CLI commands                                                        */
/* ------------------------------------------------------------------ */

XELPRESULT cmdSsid(XELP *ths, const char *args, int len)
{
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char *)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        tokCopy(gSsid, sizeof(gSsid), tok.s, tok.p);
        nvsSave();
        XelpOut(ths, "SSID set: ", 0);
        XelpOut(ths, gSsid, 0);
        XelpOut(ths, " (saved)\n", 0);
    } else {
        XelpOut(ths, "usage: ssid <name>\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdWifiPass(XELP *ths, const char *args, int len)
{
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char *)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        tokCopy(gPass, sizeof(gPass), tok.s, tok.p);
        nvsSave();
        XelpOut(ths, "Password set (saved)\n", 0);
    } else {
        XelpOut(ths, "usage: wifipass <password>\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdConnect(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    if (gSsid[0] == '\0') {
        XelpOut(ths, "Set SSID first: ssid <name>\n", 0);
        return XELP_E_ERR;
    }
    XelpOut(ths, "Connecting to ", 0);
    XelpOut(ths, gSsid, 0);
    XelpOut(ths, "...", 0);

    WiFi.begin(gSsid, gPass);

    int timeout = 40; /* 40 * 250ms = 10s */
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(250);
        XelpOut(ths, ".", 0);
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        XelpOut(ths, " OK\nIP: ", 0);
        XelpOut(ths, WiFi.localIP().toString().c_str(), 0);
        XelpOut(ths, "\n", 0);
    } else {
        XelpOut(ths, " FAILED\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdDisconnect(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    WiFi.disconnect();
    XelpOut(ths, "Disconnected\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdStatus(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    char buf[64];

    /* WiFi */
    if (WiFi.status() == WL_CONNECTED) {
        XelpOut(ths, "WiFi: connected\nSSID: ", 0);
        XelpOut(ths, gSsid, 0);
        XelpOut(ths, "\nIP:   ", 0);
        XelpOut(ths, WiFi.localIP().toString().c_str(), 0);
        snprintf(buf, sizeof(buf), "\nRSSI: %d dBm\n", WiFi.RSSI());
        XelpOut(ths, buf, 0);
    } else {
        XelpOut(ths, "WiFi: not connected\n", 0);
        if (gSsid[0]) {
            XelpOut(ths, "SSID: ", 0);
            XelpOut(ths, gSsid, 0);
            XelpOut(ths, " (saved)\n", 0);
        }
    }

    /* BLE */
    XelpOut(ths, bleConnected ? "BLE:  connected\n" : "BLE:  advertising\n", 0);

    return XELP_S_OK;
}

XELPRESULT cmdLed(XELP *ths, const char *args, int len)
{
    XelpBuf b, tok;
    XELP_XB_INIT(b, (char *)args, len);
    if (XelpTokN(&b, 1, &tok) == XELP_S_OK) {
        int val = XelpStr2Int(tok.s, (int)(tok.p - tok.s));
        digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
        XelpOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    } else {
        XelpOut(ths, "usage: led <0|1>\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdAdc(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    uint32_t acc = 0;
    for (int i = 0; i < 16; i++) {
        acc += analogReadMilliVolts(A0);
    }
    /* 1:2 voltage divider: multiply by 2, average 16 samples */
    float volts = 2.0f * acc / 16.0f / 1000.0f;
    char buf[32];
    /* dtostrf is available on ESP32 Arduino */
    dtostrf(volts, 1, 3, buf);
    XelpOut(ths, "A0: ", 0);
    XelpOut(ths, buf, 0);
    XelpOut(ths, " V\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdHelp(XELP *ths, const char *args, int len)
{
    (void)args; (void)len;
    return XelpHelp(ths);
}

/* ------------------------------------------------------------------ */
/* Command table (shared by both instances)                            */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,       "help",       "show help"                       },
    { &cmdSsid,       "ssid",       "ssid <name> -- set WiFi SSID"    },
    { &cmdWifiPass,   "wifipass",   "wifipass <pw> -- set WiFi pass"  },
    { &cmdConnect,    "connect",    "connect to WiFi"                 },
    { &cmdDisconnect, "disconnect", "disconnect from WiFi"            },
    { &cmdStatus,     "status",     "show WiFi/BLE status"            },
    { &cmdLed,        "led",        "led <0|1> -- toggle LED"         },
    { &cmdAdc,        "adc",        "read A0 voltage"                 },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* BLE callbacks                                                       */
/* ------------------------------------------------------------------ */

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *s, NimBLEConnInfo &connInfo) override {
        (void)s; (void)connInfo;
        bleConnected = true;
        Serial.println("[BLE] client connected");
    }
    void onDisconnect(NimBLEServer *s, NimBLEConnInfo &connInfo, int reason) override {
        (void)s; (void)connInfo; (void)reason;
        bleConnected = false;
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
    }
};

/* ------------------------------------------------------------------ */
/* BLE setup                                                           */
/* ------------------------------------------------------------------ */

static void bleSetup(void)
{
    NimBLEDevice::init("xelp-c6");
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pService = pServer->createService(NUS_SERVICE_UUID);

    /* TX characteristic: xelp output → phone (notify) */
    pTxChar = pService->createCharacteristic(
        NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);

    /* RX characteristic: phone → xelp input (write) */
    NimBLECharacteristic *pRxChar = pService->createCharacteristic(
        NUS_RX_UUID, NIMBLE_PROPERTY::WRITE);
    pRxChar->setCallbacks(new RxCallbacks());

    pService->start();

    NimBLEAdvertising *pAdv = NimBLEDevice::getAdvertising();
    pAdv->addServiceUUID(NUS_SERVICE_UUID);
    NimBLEDevice::startAdvertising();

    Serial.println("[BLE] advertising as \"xelp-c6\"");
}

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 2000) ; /* wait up to 2s for USB CDC */

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(A0, INPUT);

    /* Load saved credentials */
    nvsLoad();

    /* Serial CLI */
    cliSerial.begin("XIAO ESP32-C6 xelp demo\n", &serialOut);
    cliSerial.setCommands(commands);

    /* BLE CLI */
    cliBle.begin("XIAO ESP32-C6 xelp (BLE)\n", &bleOut);
    cliBle.setCommands(commands);

    /* Start BLE */
    bleSetup();

    /* Show help on serial */
    cliSerial.run("help");

    if (gSsid[0]) {
        Serial.print("Saved SSID: ");
        Serial.println(gSsid);
    }
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop()
{
    cliSerial.poll(Serial);
    /* BLE input is handled in RxCallbacks::onWrite */
}
