/*
 * arduino-cpp-example.ino -- Arduino example using the C++ Easy API.
 *
 * Demonstrates the XelpCLI wrapper with inline lambda commands:
 *   - commands({...}) with argc/argv-style lambdas (no static tables)
 *   - XelpCLI::argInt() for const-correct argument parsing
 *   - r1()/r2() register accessors for reading command return values
 *   - run() for executing startup scripts
 *   - poll(Serial) for interactive input
 *
 * Open the Serial Monitor at 115200 baud and type "help".
 *
 * For the raw C API approach, see arduino-example.
 */

#include "xelp.h"
#include "XelpArduino.h"

XelpCLI cli;

static const char *about = "Arduino C++ xelp example v1.0\n";

/* ------------------------------------------------------------------ */
/* Output callback                                                     */
/* ------------------------------------------------------------------ */

void myOutput(char c) { Serial.write(c); }

/* ------------------------------------------------------------------ */
/* Setup                                                               */
/* ------------------------------------------------------------------ */

void setup()
{
    Serial.begin(115200);
    while (!Serial) ;

    pinMode(LED_BUILTIN, OUTPUT);

    cli.begin(about, &myOutput);
    cli.output(Serial);

    cli.commands({
        {"help", "show help",
         [](XelpCLI& c, int, const char**) {
            c.help();
        }},

        {"echo", "echo <args...>",
         [](XelpCLI& c, int argc, const char** argv) {
            for (int i = 1; i < argc; i++) {
                if (i > 1) c.print(" ");
                c.print(argv[i]);
            }
            c.print("\n");
        }},

        {"led", "led <0|1> -- set LED",
         [](XelpCLI& c, int argc, const char** argv) {
            int val;
            if (XelpCLI::argInt(argv, argc, 1, &val) != XELP_S_OK) {
                c.print("usage: led <0|1>\n");
                return;
            }
            digitalWrite(LED_BUILTIN, val ? HIGH : LOW);
            c.print(val ? "LED ON\n" : "LED OFF\n");
        }},

        {"divmod", "divmod <a> <b> -- R1=a/b R2=a%b",
         [](XelpCLI& c, int argc, const char** argv) {
            int a, d;
            if (XelpCLI::argInt(argv, argc, 1, &a) != XELP_S_OK ||
                XelpCLI::argInt(argv, argc, 2, &d) != XELP_S_OK) {
                c.print("usage: divmod <a> <b>\n");
                return;
            }
            if (d == 0) {
                c.print("division by zero\n");
                return;
            }
            c.r1() = a / d;
            c.r2() = a % d;
        }},

        {"pr", "print all registers",
         [](XelpCLI& c, int, const char**) {
            char buf[64];
            snprintf(buf, sizeof(buf), "R0=%d R1=%d R2=%d R3=%d\n",
                     (int)c.r0(), (int)c.r1(), (int)c.r2(), (int)c.r3());
            c.print(buf);
        }},
    });

    cli.setPrompt("arduino>");

    /* Demonstrate registers: run divmod and read results via C++ accessors */
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
