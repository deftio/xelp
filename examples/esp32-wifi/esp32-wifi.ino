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

XELPRESULT cmdHelp(const char *args, int len)
{
    return cli.help();
}

XELPRESULT cmdSsid(const char *args, int len)
{
    XelpBuf b, tok;
    XELP_XBInit(b, (char *)args, len);
    if (XELPTokN(&b, 1, &tok) == XELP_S_OK) {
        tokCopy(gSsid, sizeof(gSsid), tok.s, tok.p);
        cli.print("SSID set: ");
        cli.print(gSsid);
        cli.print("\n");
    } else {
        cli.print("usage: ssid <name>\n");
    }
    return XELP_S_OK;
}

XELPRESULT cmdPass(const char *args, int len)
{
    XelpBuf b, tok;
    XELP_XBInit(b, (char *)args, len);
    if (XELPTokN(&b, 1, &tok) == XELP_S_OK) {
        tokCopy(gPass, sizeof(gPass), tok.s, tok.p);
        cli.print("Password set\n");
    } else {
        cli.print("usage: pass <password>\n");
    }
    return XELP_S_OK;
}

XELPRESULT cmdConnect(const char *args, int len)
{
    if (gSsid[0] == '\0') {
        cli.print("Set SSID first: ssid <name>\n");
        return XELP_E_ERR;
    }
    cli.print("Connecting to ");
    cli.print(gSsid);
    cli.print("...");

    WiFi.begin(gSsid, gPass);

    int timeout = 40; /* 40 * 250ms = 10 seconds */
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(250);
        cli.print(".");
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        cli.print(" OK\nIP: ");
        cli.print(WiFi.localIP().toString().c_str());
        cli.print("\n");
        cli.setPrompt("wifi>");
    } else {
        cli.print(" FAILED\n");
    }
    return XELP_S_OK;
}

XELPRESULT cmdDisconnect(const char *args, int len)
{
    WiFi.disconnect();
    cli.print("Disconnected\n");
    cli.setPrompt("esp32>");
    return XELP_S_OK;
}

XELPRESULT cmdStatus(const char *args, int len)
{
    if (WiFi.status() == WL_CONNECTED) {
        cli.print("WiFi: connected\nSSID: ");
        cli.print(gSsid);
        cli.print("\nIP: ");
        cli.print(WiFi.localIP().toString().c_str());
        char rssi[16];
        snprintf(rssi, sizeof(rssi), "\nRSSI: %d dBm\n", WiFi.RSSI());
        cli.print(rssi);
    } else {
        cli.print("WiFi: not connected\n");
        if (gSsid[0]) {
            cli.print("SSID: ");
            cli.print(gSsid);
            cli.print(" (saved)\n");
        }
    }
    return XELP_S_OK;
}

XELPRESULT cmdTime(const char *args, int len)
{
    if (WiFi.status() != WL_CONNECTED) {
        cli.print("Not connected. Use: connect\n");
        return XELP_E_ERR;
    }

    cli.print("Fetching time...\n");
    if (httpGet("http://worldtimeapi.org/api/ip") < 0) {
        cli.print("HTTP error\n");
        return XELP_E_ERR;
    }

    char datetime[40], tz[48];
    if (jsonStr("datetime", datetime, sizeof(datetime))) {
        cli.print(datetime);
    }
    if (jsonStr("timezone", tz, sizeof(tz))) {
        cli.print(" (");
        cli.print(tz);
        cli.print(")");
    }
    cli.print("\n");
    return XELP_S_OK;
}

XELPRESULT cmdWeather(const char *args, int len)
{
    if (WiFi.status() != WL_CONNECTED) {
        cli.print("Not connected. Use: connect\n");
        return XELP_E_ERR;
    }

    XelpBuf b, tok1, tok2;
    XELP_XBInit(b, (char *)args, len);
    XELP_XBTOP(b);
    if (XELPTokN(&b, 1, &tok1) != XELP_S_OK ||
        (XELP_XBTOP(b), XELPTokN(&b, 2, &tok2) != XELP_S_OK)) {
        cli.print("usage: weather <lat> <lon>\n");
        cli.print("  e.g. weather 37.77 -122.42\n");
        return XELP_E_ERR;
    }

    char lat[16], lon[16];
    tokCopy(lat, sizeof(lat), tok1.s, tok1.p);
    tokCopy(lon, sizeof(lon), tok2.s, tok2.p);

    char url[160];
    snprintf(url, sizeof(url),
        "http://api.open-meteo.com/v1/forecast?"
        "latitude=%s&longitude=%s&current_weather=true", lat, lon);

    cli.print("Fetching weather...\n");
    if (httpGet(url) < 0) {
        cli.print("HTTP error\n");
        return XELP_E_ERR;
    }

    char temp[12], wind[12], code[8];
    if (jsonNum("temperature", temp, sizeof(temp))) {
        cli.print("Temperature: ");
        cli.print(temp);
        cli.print(" C\n");
    }
    if (jsonNum("windspeed", wind, sizeof(wind))) {
        cli.print("Wind: ");
        cli.print(wind);
        cli.print(" km/h\n");
    }
    if (jsonNum("weathercode", code, sizeof(code))) {
        cli.print("Weather code: ");
        cli.print(code);
        int wc = atoi(code);
        if      (wc == 0)             cli.print(" (clear sky)");
        else if (wc <= 3)             cli.print(" (partly cloudy)");
        else if (wc >= 51 && wc <= 67) cli.print(" (rain)");
        else if (wc >= 71 && wc <= 77) cli.print(" (snow)");
        else if (wc >= 95)            cli.print(" (thunderstorm)");
        cli.print("\n");
    }
    return XELP_S_OK;
}

XELPRESULT cmdLed(const char *args, int len)
{
    XelpBuf b, tok;
    int val;
    XELP_XBInit(b, (char *)args, len);
    if (XELPTokN(&b, 1, &tok) == XELP_S_OK) {
        XELPParseNum(tok.s, (int)(tok.p - tok.s), &val);
        digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
        cli.print(val ? "LED ON\n" : "LED OFF\n");
    }
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
    Serial.println();
}

/* ------------------------------------------------------------------ */
/* Loop                                                                */
/* ------------------------------------------------------------------ */

void loop()
{
    cli.poll(Serial);
}
