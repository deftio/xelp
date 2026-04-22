/*
 * xelp-example-cpp.cpp -- C++ Easy API demo with ncurses.
 *
 * Same functionality as posix-simple/xelp-example.c but written using
 * the XelpCLI C++ wrapper and commands({...}) with inline lambdas.
 * No static function tables, no raw XELP* pointers.
 *
 * Build:  make            (builds and runs)
 *         make build      (build only)
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee
 * BSD-2-Clause -- see LICENSE.txt
 */

#include <stdio.h>
#include <ncurses.h>

#include "xelp.h"
#include "XelpArduino.h"

/* ------------------------------------------------------------------ */
/* ncurses output functions                                            */
/* ------------------------------------------------------------------ */

static void ncOut(char c)  { addch(c); refresh(); }

static void ncBksp() {
    int r, c;
    getyx(stdscr, r, c);
    move(r, c - 1);
    delch();
    refresh();
}

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

static XelpCLI cli;
static int     gExit = 0;

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main()
{
    /* --- ncurses setup --------------------------------------------- */
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    /*
     * keypad OFF: ncurses passes raw escape bytes through to xelp's
     * built-in key accumulator, which reassembles arrow keys, Home,
     * End, Delete, etc. into XELP_KEYCODE_* values.
     */
    keypad(stdscr, FALSE);

    /* --- xelp setup ------------------------------------------------ */
    const char *about =
        "\nxelp C++ easy-API demo\n"
        "\n"
        "ESC     : single-key mode  (x = exit, ? = help)\n"
        "CTRL-P  : CLI mode         (type command + ENTER)\n"
        "CTRL-T  : pass-through mode\n"
        "\n";

    cli.begin(about, &ncOut);
    cli.setBackspace(&ncBksp);
    cli.setErrorHandler(&ncOut);
    cli.setModeChangeHandler([](int mode) {
        const char *names[] = { "CLI", "KEY", "THR" };
        if (mode >= XELP_MODE_CLI && mode <= XELP_MODE_THR)
            printw("Mode changed to %s\n", names[mode]);
    });

    /* --- CLI commands (Easy API) ----------------------------------- */
    cli.commands({
        {"help", "show help", [](XelpCLI& c, int, const char**) {
            c.help();
        }},
        {"banner", "print xelp banner", [](XelpCLI& c, int, const char**) {
            c.print(XELP_BANNER_STR);
        }},
        {"echo", "echo args back", [](XelpCLI& c, int argc, const char** argv) {
            for (int i = 1; i < argc; i++) {
                if (i > 1) c.print(" ");
                c.print(argv[i]);
            }
            c.print("\n");
        }},
        {"cls", "clear screen", [](XelpCLI&, int, const char**) {
            clear();
            refresh();
        }},
        {"home", "cursor to top-left", [](XelpCLI&, int, const char**) {
            move(0, 0);
            refresh();
        }},
        {"divmod", "divmod <a> <b>: R1=a/b R2=a%b", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 3) { c.print("usage: divmod <a> <b>\n"); return; }
            int a = XELPStr2Int(argv[1], XELPStrLen(argv[1]));
            int b = XELPStr2Int(argv[2], XELPStrLen(argv[2]));
            if (b == 0) { c.print("divmod: division by zero\n"); return; }
            c.r1() = a / b;
            c.r2() = a % b;
            char buf[64];
            snprintf(buf, sizeof(buf), "%d / %d = %d remainder %d\n", a, b, c.r1(), c.r2());
            c.print(buf);
        }},
        {"pr", "print all registers", [](XelpCLI& c, int, const char**) {
            char buf[64];
            snprintf(buf, sizeof(buf), "R0=%d R1=%d R2=%d R3=%d\n",
                     c.r0(), c.r1(), c.r2(), c.r3());
            c.print(buf);
        }},
        {"num", "print integer to console", [](XelpCLI& c, int argc, const char** argv) {
            if (argc < 2) { c.print("usage: num <integer>\n"); return; }
            int val = XELPStr2Int(argv[1], XELPStrLen(argv[1]));
            char buf[32];
            snprintf(buf, sizeof(buf), "%d\n", val);
            c.print(buf);
        }},
        {"exit", "quit demo program", [](XelpCLI&, int, const char**) {
            gExit = 1;
        }},
    });

    /* --- Key commands (Easy API) ----------------------------------- */
    cli.keyCommands({
        {'?', "show help",       [](XelpCLI& c, XELPKEYCODE) { c.help(); }},
        {'b', "print banner",    [](XelpCLI& c, XELPKEYCODE) { c.print(XELP_BANNER_STR); }},
        {'x', "exit",            [](XelpCLI&,   XELPKEYCODE) { gExit = 1; }},
    });

    /* --- Ready ----------------------------------------------------- */
    printw("\n============================================================\n");
    cli.print(XELP_BANNER_STR);
    cli.help();
    printw("\nXELP struct size: %d bytes\n", (int)sizeof(XELP));
    cli.parseKey('\n');  /* emit first prompt */

    /* --- Main loop ------------------------------------------------- */
    int ch;
    do {
        ch = getch();
        if (ch != ERR)
            cli.parseKey((char)ch);
    } while (!gExit);

    endwin();
    printf("\n");
    return 0;
}
