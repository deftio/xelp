/*
 * esp32-wifi-example.ino -- ESP32 WiFi CLI with time and weather fetch.
 *
 * Configure WiFi credentials over the serial CLI, then fetch the
 * current time and weather from free APIs -- no API keys needed.
 *
 * Requires: ESP32 Arduino core (WiFi.h, HTTPClient.h ship with it).
 * No other external libraries.
 *
 * Open Serial Monitor at 115200 baud and type "help".
 *
 * Example session:
 *   esp32> ssid MyNetwork
 *   esp32> pass MyPassword
 *   esp32> connect
 *   wifi> time
 *   wifi> weather 37.77 -122.42
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include "xelp.h"
#include "XelpArduino.h"

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

XelpCLI cli;

static char gSsid[33]  = "";
static char gPass[65]   = "";
static char gHttpBuf[1024];

void myOutput(char c) { Serial.write(c); }

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

/* Copy a token (start..end) into a dest buffer, null-terminate. */
static void tokCopy(char *dest, int maxlen, const char *s, const char *e)
{
    int len = e - s;
    if (len >= maxlen) len = maxlen - 1;
    memcpy(dest, s, len);
    dest[len] = '\0';
}

/* HTTP GET into gHttpBuf. Returns body length or -1 on error. */
static int httpGet(const char *url)
{
    HTTPClient http;
    http.begin(url);
    int code = http.GET();
    if (code != 200) {
        http.end();
        return -1;
    }
    String body = http.getString();
    int len = body.length();
    if (len >= (int)sizeof(gHttpBuf)) len = sizeof(gHttpBuf) - 1;
    memcpy(gHttpBuf, body.c_str(), len);
    gHttpBuf[len] = '\0';
    http.end();
    return len;
}

/*
 * Extract a JSON string value for a given key from gHttpBuf.
 * Writes into dest, returns true on success.
 * Only handles simple "key":"value" (no nesting, no escapes).
 */
static bool jsonStr(const char *key, char *dest, int maxlen)
{
    char needle[48];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(gHttpBuf, needle);
    if (!p) return false;
    p += strlen(needle);
    while (*p == ' ') p++;
    if (*p == '"') {
        p++;
        const char *q = strchr(p, '"');
        if (!q) return false;
        tokCopy(dest, maxlen, p, q);
        return true;
    }
    return false;
}

/*
 * Extract a JSON numeric value for a given key from gHttpBuf.
 * Handles integers and decimals. Writes the raw text into dest.
 */
static bool jsonNum(const char *key, char *dest, int maxlen)
{
    char needle[48];
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    const char *p = strstr(gHttpBuf, needle);
    if (!p) return false;
    p += strlen(needle);
    while (*p == ' ') p++;
    const char *start = p;
    while (*p == '-' || *p == '.' || (*p >= '0' && *p <= '9')) p++;
    if (p == start) return false;
    tokCopy(dest, maxlen, start, p);
    return true;
}

/* ------------------------------------------------------------------ */
/* CLI commands                                                        */
/* ------------------------------------------------------------------ */

XELPRESULT cmdHelp(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

XELPRESULT cmdSsid(XELP *ths, int argc, const char **argv)
{
    if (argc > 1) {
        int slen = XelpStrLen(argv[1]);
        if (slen >= (int)sizeof(gSsid)) slen = sizeof(gSsid) - 1;
        memcpy(gSsid, argv[1], slen);
        gSsid[slen] = '\0';
        XelpOut(ths, "SSID set: ", 0);
        XelpOut(ths, gSsid, 0);
        XelpOut(ths, "\n", 0);
    } else {
        XelpOut(ths, "usage: ssid <name>\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdPass(XELP *ths, int argc, const char **argv)
{
    if (argc > 1) {
        int plen = XelpStrLen(argv[1]);
        if (plen >= (int)sizeof(gPass)) plen = sizeof(gPass) - 1;
        memcpy(gPass, argv[1], plen);
        gPass[plen] = '\0';
        XelpOut(ths, "Password set\n", 0);
    } else {
        XelpOut(ths, "usage: pass <password>\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdConnect(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    if (gSsid[0] == '\0') {
        XelpOut(ths, "Set SSID first: ssid <name>\n", 0);
        return XELP_E_ERR;
    }
    XelpOut(ths, "Connecting to ", 0);
    XelpOut(ths, gSsid, 0);
    XelpOut(ths, "...", 0);

    WiFi.begin(gSsid, gPass);

    int timeout = 40; /* 40 * 250ms = 10 seconds */
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(250);
        XelpOut(ths, ".", 0);
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        XelpOut(ths, " OK\nIP: ", 0);
        XelpOut(ths, WiFi.localIP().toString().c_str(), 0);
        XelpOut(ths, "\n", 0);
        cli.setPrompt("wifi>");
    } else {
        XelpOut(ths, " FAILED\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdDisconnect(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    WiFi.disconnect();
    XelpOut(ths, "Disconnected\n", 0);
    cli.setPrompt("esp32>");
    return XELP_S_OK;
}

XELPRESULT cmdStatus(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    if (WiFi.status() == WL_CONNECTED) {
        XelpOut(ths, "WiFi: connected\nSSID: ", 0);
        XelpOut(ths, gSsid, 0);
        XelpOut(ths, "\nIP: ", 0);
        XelpOut(ths, WiFi.localIP().toString().c_str(), 0);
        char rssi[16];
        snprintf(rssi, sizeof(rssi), "\nRSSI: %d dBm\n", WiFi.RSSI());
        XelpOut(ths, rssi, 0);
    } else {
        XelpOut(ths, "WiFi: not connected\n", 0);
        if (gSsid[0]) {
            XelpOut(ths, "SSID: ", 0);
            XelpOut(ths, gSsid, 0);
            XelpOut(ths, " (saved)\n", 0);
        }
    }
    return XELP_S_OK;
}

XELPRESULT cmdTime(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    if (WiFi.status() != WL_CONNECTED) {
        XelpOut(ths, "Not connected. Use: connect\n", 0);
        return XELP_E_ERR;
    }

    XelpOut(ths, "Fetching time...\n", 0);
    if (httpGet("http://worldtimeapi.org/api/ip") < 0) {
        XelpOut(ths, "HTTP error\n", 0);
        return XELP_E_ERR;
    }

    char datetime[40], tz[48];
    if (jsonStr("datetime", datetime, sizeof(datetime))) {
        XelpOut(ths, datetime, 0);
    }
    if (jsonStr("timezone", tz, sizeof(tz))) {
        XelpOut(ths, " (", 0);
        XelpOut(ths, tz, 0);
        XelpOut(ths, ")", 0);
    }
    XelpOut(ths, "\n", 0);
    return XELP_S_OK;
}

XELPRESULT cmdWeather(XELP *ths, int argc, const char **argv)
{
    if (WiFi.status() != WL_CONNECTED) {
        XelpOut(ths, "Not connected. Use: connect\n", 0);
        return XELP_E_ERR;
    }

    if (argc < 3) {
        XelpOut(ths, "usage: weather <lat> <lon>\n", 0);
        XelpOut(ths, "  e.g. weather 37.77 -122.42\n", 0);
        return XELP_E_ERR;
    }

    char url[160];
    snprintf(url, sizeof(url),
        "http://api.open-meteo.com/v1/forecast?"
        "latitude=%s&longitude=%s&current_weather=true", argv[1], argv[2]);

    XelpOut(ths, "Fetching weather...\n", 0);
    if (httpGet(url) < 0) {
        XelpOut(ths, "HTTP error\n", 0);
        return XELP_E_ERR;
    }

    char temp[12], wind[12], code[8];
    if (jsonNum("temperature", temp, sizeof(temp))) {
        XelpOut(ths, "Temperature: ", 0);
        XelpOut(ths, temp, 0);
        XelpOut(ths, " C\n", 0);
    }
    if (jsonNum("windspeed", wind, sizeof(wind))) {
        XelpOut(ths, "Wind: ", 0);
        XelpOut(ths, wind, 0);
        XelpOut(ths, " km/h\n", 0);
    }
    if (jsonNum("weathercode", code, sizeof(code))) {
        XelpOut(ths, "Weather code: ", 0);
        XelpOut(ths, code, 0);
        int wc = atoi(code);
        if      (wc == 0)             XelpOut(ths, " (clear sky)", 0);
        else if (wc <= 3)             XelpOut(ths, " (partly cloudy)", 0);
        else if (wc >= 51 && wc <= 67) XelpOut(ths, " (rain)", 0);
        else if (wc >= 71 && wc <= 77) XelpOut(ths, " (snow)", 0);
        else if (wc >= 95)            XelpOut(ths, " (thunderstorm)", 0);
        XelpOut(ths, "\n", 0);
    }
    return XELP_S_OK;
}

XELPRESULT cmdLed(XELP *ths, int argc, const char **argv)
{
    if (argc > 1) {
        int val;
        XelpParseNum(argv[1], XelpStrLen(argv[1]), &val);
        digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
        XelpOut(ths, val ? "LED ON\n" : "LED OFF\n", 0);
    }
    return XELP_S_OK;
}

/* Integer division: quotient -> R1, remainder -> R2 */
XELPRESULT cmdDivmod(XELP *ths, int argc, const char **argv)
{
    if (argc < 3) {
        XelpOut(ths, "usage: divmod <a> <b>\n", 0);
        return XELP_E_ERR;
    }

    int a = XelpStr2Int(argv[1], XelpStrLen(argv[1]));
    int d = XelpStr2Int(argv[2], XelpStrLen(argv[2]));
    char buf[48];

    if (d == 0) {
        XelpOut(ths, "division by zero\n", 0);
        return XELP_E_ERR;
    }

    ths->mR[1] = a / d;
    ths->mR[2] = a % d;
    snprintf(buf, sizeof(buf), "%d / %d = %d remainder %d\n",
             a, d, ths->mR[1], ths->mR[2]);
    XelpOut(ths, buf, 0);
    return XELP_S_OK;
}

/* Print all 4 registers */
XELPRESULT cmdPrintR(XELP *ths, int argc, const char **argv)
{
    char buf[64];
    (void)argc; (void)argv;
    snprintf(buf, sizeof(buf), "R0=%d R1=%d R2=%d R3=%d\n",
             XELP_R0(*ths), XELP_R1(*ths), XELP_R2(*ths), XELP_R3(*ths));
    XelpOut(ths, buf, 0);
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmdHelp,       "help",       "show help"                    },
    { &cmdSsid,       "ssid",       "ssid <name> -- set WiFi SSID" },
    { &cmdPass,       "pass",       "pass <pw> -- set WiFi password" },
    { &cmdConnect,    "connect",    "connect to WiFi"              },
    { &cmdDisconnect, "disconnect", "disconnect from WiFi"         },
    { &cmdStatus,     "status",     "show WiFi status"             },
    { &cmdTime,       "time",       "fetch current time"           },
    { &cmdWeather,    "weather",    "weather <lat> <lon> -- fetch weather" },
    { &cmdLed,        "led",        "led <0|1> -- set LED"         },
    { &cmdDivmod,     "divmod",     "divmod <a> <b> -- R1=a/b R2=a%b" },
    { &cmdPrintR,     "pr",         "print all registers"          },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(LED_BUILTIN, OUTPUT);

    cli.begin("ESP32 WiFi CLI v1.0\n", &myOutput);
    cli.setCommands(commands);
    cli.setPrompt("esp32>");

    cli.run("echo Type help for commands");

    /* Demonstrate registers: run divmod and read results via C++ accessors */
    cli.run("divmod 100 7");
    Serial.print("100/7 = ");
    Serial.print(cli.r1());       /* quotient  -> 14 */
    Serial.print(" remainder ");
    Serial.println(cli.r2());     /* remainder -> 2  */

    Serial.println();
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop()
{
    cli.poll(Serial);
}
