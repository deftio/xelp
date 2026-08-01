/* script-demo.c — demonstrates xelp script engine round-trip:
   script vars -> user CLI functions -> results back in script vars */
#include "xelp.h"
#include <stdio.h>

static void myOut(char c) { putchar(c); }

/* --- User CLI commands that interact with script variables --- */

/* "double <varname>" — reads $varname (already expanded), doubles it,
   writes result back into script var "_result" */
static XELPRESULT cmd_double(XELP *ths, int argc, const char **argv)
{
    int val;
    char cmd[48];
    int pos;
    if (argc < 2) return XELP_E_ERR;
    if (XELP_S_OK != XelpParseNum(argv[1], XelpStrLen(argv[1]), &val))
        return XELP_E_ERR;
    val *= 2;
    /* build "_set _result <val>" and execute it */
    pos = 0;
    { const char *p = "_set _result "; while (*p) cmd[pos++] = *p++; }
    /* int to string inline */
    {
        char tmp[12];
        int ti = 0, neg = 0;
        unsigned int uv;
        if (val < 0) { neg = 1; uv = (unsigned)(-(val+1))+1u; }
        else uv = (unsigned)val;
        if (uv == 0) tmp[ti++] = '0';
        else while (uv) { tmp[ti++] = (char)('0'+(uv%10)); uv /= 10; }
        if (neg) cmd[pos++] = '-';
        while (ti > 0) cmd[pos++] = tmp[--ti];
    }
    cmd[pos] = '\0';
    XelpParse(ths, cmd, pos);
    return XELP_S_OK;
}

/* "sum3 a b c" — sum three expanded integer args, store in _result */
static XELPRESULT cmd_sum3(XELP *ths, int argc, const char **argv)
{
    int a, b, c, pos;
    char cmd[48], tmp[12];
    int ti, neg;
    unsigned int uv;
    if (argc < 4) return XELP_E_ERR;
    if (XELP_S_OK != XelpParseNum(argv[1], XelpStrLen(argv[1]), &a)) return XELP_E_ERR;
    if (XELP_S_OK != XelpParseNum(argv[2], XelpStrLen(argv[2]), &b)) return XELP_E_ERR;
    if (XELP_S_OK != XelpParseNum(argv[3], XelpStrLen(argv[3]), &c)) return XELP_E_ERR;
    a = a + b + c;
    pos = 0;
    { const char *p = "_set _result "; while (*p) cmd[pos++] = *p++; }
    ti = 0; neg = 0;
    if (a < 0) { neg = 1; uv = (unsigned)(-(a+1))+1u; } else uv = (unsigned)a;
    if (uv == 0) tmp[ti++] = '0';
    else while (uv) { tmp[ti++] = (char)('0'+(uv%10)); uv /= 10; }
    if (neg) cmd[pos++] = '-';
    while (ti > 0) cmd[pos++] = tmp[--ti];
    cmd[pos] = '\0';
    XelpParse(ths, cmd, pos);
    return XELP_S_OK;
}

static XELPCLIFuncMapEntry gCmds[] = {
    { cmd_double, "double", "double a value"     },
    { cmd_sum3,   "sum3",   "sum three values"   },
    XELP_FUNC_ENTRY_LAST
};

/* helper: run a script string and print a header */
static void run(XELP *x, const char *label, const char *script)
{
    printf("--- %s ---\n", label);
    printf("  script: %s\n", script);
    printf("  output: ");
    XelpParse(x, script, XelpStrLen(script));
    printf("\n");
}

int main(void)
{
    XELP x;
    XelpInit(&x, "script-demo");
    x.mpfOut  = myOut;
    x.mpCLIModeFuncs = gCmds;

    printf("=== Xelp Script Engine Demo ===\n\n");

    /* 1. basic vars and math */
    run(&x, "Basic math",
        "_set x 10; _set y 20; _add z $x $y; _print $x + $y = $z");

    /* 2. chained computation */
    run(&x, "Chained ops",
        "_set a 100; _mul b $a 3; _sub c $b 50; _div d $c 5; _print (100*3-50)/5 = $d");

    /* 3. comparisons */
    run(&x, "Comparisons",
        "_set score 85; _set pass 70; "
        "_gt ok $score $pass; _print 85>70? $ok; "
        "_eq same $score $score; _print 85==85? $same; "
        "_lt less $score $pass; _print 85<70? $less");

    /* 4. pass var to user CLI "double" command, get result back */
    run(&x, "CLI round-trip: double",
        "_set val 21; double $val; _print double($val) = $_result");

    /* 5. pass multiple vars to user CLI "sum3" command */
    run(&x, "CLI round-trip: sum3",
        "_set a 10; _set b 20; _set c 30; sum3 $a $b $c; _print sum3($a,$b,$c) = $_result");

    /* 6. use result from one CLI call in the next computation */
    run(&x, "Result chaining",
        "_set n 7; double $n; _set doubled $_result; "
        "_add final $doubled 100; _print double(7)+100 = $final");

    /* 7. type introspection */
    run(&x, "Type system",
        "_set num 42; _set str hello; _type num; _type str; _type missing");

    /* 8. frame push/pop — new frame is isolated (no var inheritance) */
    run(&x, "Frame scoping",
        "_set outer 100; _push; _set inner 999; "
        "_print inner-frame: $inner; "
        "_pop; _print back-in-root: $outer");

    /* 9. negative and hex values */
    run(&x, "Negative & hex",
        "_set a -15; _set b 0x1F; _add c $a $b; _print -15 + 0x1F = $c");

    /* 10. multi-step algorithm: compute n*(n+1)/2 */
    run(&x, "Triangle number (n=10)",
        "_set n 10; _add n1 $n 1; _mul prod $n $n1; _div tri $prod 2; "
        "_print triangle(10) = $tri");

    return 0;
}
