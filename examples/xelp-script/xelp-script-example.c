/*
 * xelp-script-example.c -- XelpScript engine demonstration.
 *
 * Non-interactive POSIX example (no ncurses) showing XelpScript
 * features via XelpParse().  Covers:
 *
 *   1. Variables + print       (_set, $ expansion, _print)
 *   2. Math builtins           (_add, _sub, _mul, _div, _mod, _inc, _dec)
 *   3. Paren subexpressions    (_set area (_mul (_add $w 1) $h))
 *   4. Comparison + logic      (_eq, _lt, _not, _and)
 *   5. Conditionals            (_if $x _then ... _else ...)
 *   6. Labels + loops          (:label, _goto, breakpoint callback)
 *   7. Functions               (C-registered ROM script funcs, @1/@2 params)
 *   8. C interop               (C command calls XelpCallProc for script func)
 *   9. Register access         (_mr for reading/writing mR[])
 *
 * Build (from repo root):
 *   gcc -Wall -Wextra -Isrc examples/xelp-script/xelp-script-example.c \
 *       src/xelp.c -o xelp-script-example
 */

#include <stdio.h>
#include "xelp.h"

/* ------------------------------------------------------------------ */
/* Platform abstraction                                                */
/* ------------------------------------------------------------------ */

static void out_putc(char c) { putchar(c); }

/* ------------------------------------------------------------------ */
/* Breakpoint callback -- limits loop iterations for safety            */
/* ------------------------------------------------------------------ */

static int g_step_budget;

static XELPRESULT breakpoint_cb(XELP *ths)
{
    (void)ths;
    if (--g_step_budget <= 0)
        return XELP_E_ERR;   /* halt execution */
    return XELP_S_OK;
}

/* ------------------------------------------------------------------ */
/* Command handlers                                                    */
/* ------------------------------------------------------------------ */

static XELP cli;

static XELPRESULT cmd_echo(XELP *ths, int argc, const char **argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        if (i > 1) XelpOut(ths, " ", 0);
        XelpOut(ths, argv[i], 0);
    }
    XelpOut(ths, "\n", 0);
    return XELP_S_OK;
}

static XELPRESULT cmd_show(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    printf("  R0=%d R1=%d R2=%d R3=%d\n",
           (int)XELP_R0(*ths), (int)XELP_R1(*ths),
           (int)XELP_R2(*ths), (int)XELP_R3(*ths));
    return XELP_S_OK;
}

/*
 * callscript -- C command that invokes a script function via XelpCallProc.
 * Usage: callscript <funcname> [args...]
 * Demonstrates C -> script interop.
 */
static XELPRESULT cmd_callscript(XELP *ths, int argc, const char **argv)
{
    char buf[64];
    int i, pos;

    if (argc < 2) {
        XelpOut(ths, "usage: callscript <func> [args...]\n", 0);
        return XELP_E_ERR;
    }

    /* Build a command line from argv[1..] */
    pos = 0;
    for (i = 1; i < argc && pos < 62; i++) {
        const char *s = argv[i];
        if (i > 1 && pos < 62) buf[pos++] = ' ';
        while (*s && pos < 62)
            buf[pos++] = *s++;
    }
    buf[pos] = '\0';

    return XelpCallProc(ths, buf);
}

static XELPRESULT cmd_help(XELP *ths, int argc, const char **argv)
{
    (void)argc; (void)argv;
    return XelpHelp(ths);
}

/* ------------------------------------------------------------------ */
/* Command table                                                       */
/* ------------------------------------------------------------------ */

XELPCLIFuncMapEntry commands[] = {
    { &cmd_help,       "help",       "show help"                         },
    { &cmd_echo,       "echo",       "echo arguments"                    },
    { &cmd_show,       "show",       "print registers R0-R3"             },
    { &cmd_callscript, "callscript", "callscript <func> [args]"          },
    XELP_FUNC_ENTRY_LAST
};

/* ------------------------------------------------------------------ */
/* C-registered ROM script functions (no malloc, lives in .rodata)     */
/* These can be called from the CLI or from other scripts.             */
/* @1, @2 etc. are positional parameters.                              */
/* ------------------------------------------------------------------ */

XELPScriptFuncEntry script_funcs[] = {
    { "double", "_return (_mul @1 2)", "double <n> -- return n*2" },
    { "add2",   "_return (_add @1 @2)", "add2 <a> <b> -- return a+b" },
    { "triple", "_return (_mul @1 3)", "triple <n> -- return n*3" },
    { 0, 0, 0 }
};

/* ------------------------------------------------------------------ */
/* Helper: run a script with a section header + step budget            */
/* Each section reinitializes the instance so the arena is fresh.      */
/* ------------------------------------------------------------------ */

static void run_section(const char *title, const char *script)
{
    printf("\n=== %s ===\n", title);
    XelpInit(&cli, "XelpScript Example v1.0\n");
    XELP_SET_FN_OUT(cli, &out_putc);
    XELP_SET_FN_CLI(cli, commands);
    XELP_SET_FN_SCRIPT(cli, script_funcs);
    XELP_SET_FN_BREAKPOINT(cli, &breakpoint_cb);
    g_step_budget = 500;
    XelpParse(&cli, script, XelpStrLen(script));
}

/* ================================================================== */
/* main                                                                */
/* ================================================================== */

int main(void)
{
    printf("%s\n", XELP_BANNER_STR);
    printf("XelpScript Engine Demo\n");

    /* =============================================================== */
    /* 1. Variables + print                                            */
    /*    _set creates a variable, $name reads it, _print outputs it.  */
    /* =============================================================== */

    run_section("1. Variables + print",
        "_set x 42\n"
        "_print $x\n"
        "echo ---\n"
        "_set msg hello\n"
        "_print $msg\n"
    );

    /* =============================================================== */
    /* 2. Math builtins                                                */
    /*    _add/_sub/_mul/_div/_mod return results via the result stack. */
    /*    Use parens with _set to capture: _set r (_add $a $b)         */
    /*    _inc/_dec modify a variable in place.                        */
    /* =============================================================== */

    run_section("2. Math builtins",
        "_set a 10\n"
        "_set b 3\n"
        "_set r (_add $a $b)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_sub $a $b)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_mul $a $b)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_div $a $b)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_mod $a $b)\n"
        "_print $r\n"
        "echo ---\n"
        "_inc a\n"
        "_dec b\n"
        "_print $a\n"
        "_print $b\n"
    );

    /* =============================================================== */
    /* 3. Paren subexpressions                                         */
    /*    Nested parentheses are evaluated inside-out.                  */
    /*    (_mul (_add $w 1) $h) => _add 4 1 => 5, _mul 5 5 => 25      */
    /* =============================================================== */

    run_section("3. Paren subexpressions",
        "_set w 4\n"
        "_set h 5\n"
        "_set area (_mul (_add $w 1) $h)\n"
        "_print $area\n"
    );

    /* =============================================================== */
    /* 4. Comparison + logic                                           */
    /*    _eq/_lt return 1 or 0. _not/_and combine results.            */
    /* =============================================================== */

    run_section("4. Comparison + logic",
        "_set x 10\n"
        "_set y 20\n"
        "_set r (_eq $x $y)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_lt $x $y)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (_not 0)\n"
        "_print $r\n"
    );

    /* =============================================================== */
    /* 5. Conditionals                                                 */
    /*    _if <cond> _then <cmd> [_else <cmd>]                         */
    /* =============================================================== */

    run_section("5. Conditionals",
        "_set t 75\n"
        "_if (_lt $t 100) _then echo OK _else echo OVER\n"
        "_set t 120\n"
        "_if (_lt $t 100) _then echo OK _else echo OVER\n"
    );

    /* =============================================================== */
    /* 6. Labels + loops                                               */
    /*    :label defines a jump target. _goto :label jumps back.       */
    /*    :_end marks the end of the script (required for _goto).      */
    /*    Breakpoint callback prevents infinite loops.                  */
    /* =============================================================== */

    run_section("6. Labels + loops",
        "_set i 0\n"
        ":top\n"
        "_print $i\n"
        "_inc i\n"
        "_if (_lt $i 5) _then _goto :top\n"
        "echo done\n"
        ":_end\n"
    );

    /* =============================================================== */
    /* 7. C-registered ROM script functions                            */
    /*    XELPScriptFuncEntry table defines functions in .rodata.       */
    /*    @1, @2 are positional params. _return sets the result.       */
    /*    Call with parens to capture return value: _set r (double 7)   */
    /* =============================================================== */

    run_section("7. ROM script functions",
        "_set r (double 7)\n"
        "_print $r\n"
        "echo ---\n"
        "_set r (add2 3 4)\n"
        "_print $r\n"
    );

    /* =============================================================== */
    /* 8. C interop -- C command invokes a script function             */
    /*    The callscript C command uses XelpCallProc() to call a       */
    /*    script function from C code. This shows bidirectional        */
    /*    interop: C -> script and script -> C commands.                */
    /* =============================================================== */

    run_section("8. C interop",
        "callscript triple 5\n"
        "show\n"
    );

    /* =============================================================== */
    /* 9. Register access                                              */
    /*    _mr <n> <val> writes to mR[n]. (_mr <n>) reads it.          */
    /* =============================================================== */

    run_section("9. Register access",
        "_mr 1 42\n"
        "_mr 2 99\n"
        "show\n"
        "_set v (_mr 1)\n"
        "_print $v\n"
    );

    /* =============================================================== */
    /* Done                                                            */
    /* =============================================================== */

    printf("\n=== XelpScript demo complete ===\n");
    return 0;
}
