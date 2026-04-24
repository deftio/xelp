/*
 * XelpArduino.h -- Header-only C++ wrapper for the xelp CLI library.
 *
 * Provides a thin XelpCLI class that wraps the raw C XELP struct.
 * Designed for Arduino but also compiles on desktop C++ (without poll()).
 *
 * Two ways to register commands:
 *   C way:   setCommands() with a static XELPCLIFuncMapEntry array.
 *   C++ way: commands({...}) with inline lambdas (Easy API).
 *
 * These are mutually exclusive: do not mix setCommands() with commands().
 *
 * Usage:
 *   #include "xelp.h"
 *   #include "XelpArduino.h"
 *
 * Copyright (C) 2011-2026  M. A. Chatterjee <deftio [at] deftio [dot] com>
 * BSD-2-Clause -- see xelp.h for full license text.
 */

#ifndef XELP_ARDUINO_H
#define XELP_ARDUINO_H

#include "xelp.h"
#include <stddef.h>           /* offsetof */
#include <initializer_list>   /* std::initializer_list */

#ifdef ARDUINO
#include <Arduino.h>
#endif

/* Capacity limits for Easy API command registration.
   Override before including this header to increase. */
#ifndef XELP_MAX_CLI_CMDS
#define XELP_MAX_CLI_CMDS 16
#endif
#ifndef XELP_MAX_KEY_CMDS
#define XELP_MAX_KEY_CMDS 8
#endif
#ifndef XELP_MAX_EASY_ARGV
#define XELP_MAX_EASY_ARGV 8
#endif

class XelpCLI;

/* Easy API callback signatures */
typedef void (*XelpEasyCliFn)(XelpCLI&, int, const char**);
typedef void (*XelpEasyKeyFn)(XelpCLI&, XELPKEYCODE);

/* Descriptor for bulk CLI command registration via commands({...}). */
struct XelpCmd {
    const char*    name;
    const char*    help;
    XelpEasyCliFn  fn;
};

/* Descriptor for bulk key command registration via keyCommands({...}). */
struct XelpKeyCmd {
    XELPKEYCODE    key;
    const char*    help;
    XelpEasyKeyFn  fn;
};

class XelpCLI {
public:
    /* ---- Initialization ------------------------------------------- */

    /**
     * Initialize the xelp instance.
     * Call this in setup() after Serial.begin().
     *
     * @param aboutMsg  Null-terminated about/version string (stored by pointer).
     * @param outputFn  Function that emits one character (e.g. Serial.write).
     *                  Pass nullptr if using output(Stream&) instead.
     */
    void begin(const char* aboutMsg, void (*outputFn)(char))
    {
        XelpInit(&m_x, aboutMsg);
        if (outputFn)
            XELP_SET_FN_OUT(m_x, outputFn);
    }

    /* ---- Command tables (Tier 1 -- static arrays) ----------------- */

#ifdef XELP_ENABLE_CLI
    /** Set the CLI command table (static array, terminated by XELP_FUNC_ENTRY_LAST). */
    void setCommands(XELPCLIFuncMapEntry* table)
    {
        XELP_SET_FN_CLI(m_x, table);
    }
#endif

#ifdef XELP_ENABLE_KEY
    /** Set the single-key command table (static array, terminated by XELP_FUNC_ENTRY_LAST). */
    void setKeyCommands(XELPKeyFuncMapEntry* table)
    {
        XELP_SET_FN_KEY(m_x, table);
    }
#endif

    /* ---- Easy API (C++ way) --------------------------------------- */

#ifdef XELP_ENABLE_CLI
    /** Register all CLI commands at once. */
    bool commands(std::initializer_list<XelpCmd> cmds)
    {
        for (const auto& c : cmds)
            if (!_addCmd(c.name, c.help, c.fn)) return false;
        return true;
    }

    /** Set the unknown-command handler (easy signature). */
    void onUnknown(XelpEasyCliFn fn)
    {
        s_unknownFn = fn;
        XELP_SET_FN_DEF_CLI(m_x, &_unknownDispatch);
    }
#endif

#ifdef XELP_ENABLE_KEY
    /** Register all key commands at once. */
    bool keyCommands(std::initializer_list<XelpKeyCmd> cmds)
    {
        for (const auto& c : cmds)
            if (!_addKey(c.key, c.help, c.fn)) return false;
        return true;
    }
#endif

    /* ---- Optional callbacks --------------------------------------- */

#ifdef XELP_CLI_PROMPT
    /**
     * Set the CLI prompt string (e.g. "mydev>").
     * Only effective when xelpcfg.h defines XELP_CLI_PROMPT as (ths->mpPrompt).
     * If XELP_CLI_PROMPT is a fixed string literal, this call has no effect.
     */
    void setPrompt(const char* p)
    {
        XELP_SET_VAL_CLI_PROMPT(m_x, p);
    }

    /** Convenience alias for setPrompt(). */
    void prompt(const char* p) { setPrompt(p); }
#endif

#ifdef XELP_ENABLE_CLI
    /** Set the destructive-backspace handler for the CLI prompt. */
    void setBackspace(void (*fn)())
    {
        XELP_SET_FN_BKSP(m_x, fn);
    }

    /** Set the default handler for unrecognized CLI commands (raw C signature). */
    void setDefaultCommandHandler(XELPRESULT (*fn)(XELP*, const char*, int))
    {
        XELP_SET_FN_DEF_CLI(m_x, fn);
    }
#endif

#ifdef XELP_ENABLE_THR
    /** Set the pass-through callback (called for each key in THR mode). */
    void setPassThru(void (*fn)(char))
    {
        XELP_SET_FN_THR(m_x, fn);
    }
#endif

#ifdef XELP_ENABLE_KEY
    /** Set the default handler for unmapped single-key presses. */
    void setDefaultKeyHandler(XELPRESULT (*fn)(XELP*, XELPKEYCODE))
    {
        XELP_SET_FN_DEF_KEY(m_x, fn);
    }
#endif

    /** Set the error callback (called when xelp encounters an error). */
    void setErrorHandler(void (*fn)(char))
    {
        XELP_SET_FN_ERR(m_x, fn);
    }

    /** Set the mode-change callback (called when switching CLI/KEY/THR). */
    void setModeChangeHandler(void (*fn)(int))
    {
        XELP_SET_FN_EMCHG(m_x, fn);
    }

    /* ---- Stream-based output (Arduino only) ----------------------- */

#ifdef ARDUINO
    /**
     * Bind a Stream for automatic output and backspace handling.
     * Replaces the output function pointer set in begin().
     *
     * Note: uses a static pointer, so only one XelpCLI instance at a
     * time can use stream output. For multi-instance, use begin() with
     * per-instance output functions instead.
     */
    void output(Stream& stream)
    {
        s_outStream = &stream;
        XELP_SET_FN_OUT(m_x, &_streamOut);
#ifdef XELP_ENABLE_CLI
        XELP_SET_FN_BKSP(m_x, &_streamBksp);
#endif
    }
#endif

    /* ---- Runtime -------------------------------------------------- */

#ifdef ARDUINO
    /**
     * Drain all available bytes from a Stream and feed them to xelp.
     * Call this in loop().
     *
     * Works with Serial, SoftwareSerial, WiFiClient, or any Stream subclass.
     */
    void poll(Stream& stream)
    {
        while (stream.available() > 0) {
            char c = (char)stream.read();
            XelpParseKey(&m_x, c);
        }
    }
#endif

#ifdef XELP_ENABLE_CLI
    /**
     * Execute a script (one or more semicolon/newline-separated commands).
     * The buffer is not modified and can live in ROM/flash.
     */
    XELPRESULT run(const char* script)
    {
        return XelpParse(&m_x, script, XelpStrLen(script));
    }
#endif

    /** Process a single keypress through xelp's interactive parser. */
    XELPRESULT parseKey(char key)
    {
        return XelpParseKey(&m_x, key);
    }

    /** Print a null-terminated string through xelp's output function. */
    void print(const char* msg)
    {
        XelpOut(&m_x, msg, XelpStrLen(msg));
    }

#ifdef XELP_ENABLE_HELP
    /** Print the built-in help listing. */
    XELPRESULT help()
    {
        return XelpHelp(&m_x);
    }
#endif

    /* ---- Registers ------------------------------------------------ */

    /** Read command status (engine-owned, read-only). */
    XELPREG  r0() const { return m_x.mR[0]; }

    /** Read/write command-specific return registers. */
    XELPREG  r1() const { return m_x.mR[1]; }
    XELPREG  r2() const { return m_x.mR[2]; }
    XELPREG  r3() const { return m_x.mR[3]; }
    XELPREG& r1()       { return m_x.mR[1]; }
    XELPREG& r2()       { return m_x.mR[2]; }
    XELPREG& r3()       { return m_x.mR[3]; }

    /* ---- Escape hatch --------------------------------------------- */

    /** Access the underlying C struct for anything the wrapper doesn't expose. */
    XELP*       raw()       { return &m_x; }
    const XELP* raw() const { return &m_x; }

private:
    /* m_x MUST remain the first member so that _fromRaw() works. */
    XELP m_x;

#ifdef XELP_ENABLE_CLI
    XELPCLIFuncMapEntry  m_cliTable[XELP_MAX_CLI_CMDS + 1]; /* +1 sentinel */
    int                  m_nCli = 0;
    XelpEasyCliFn        m_easyCliFns[XELP_MAX_CLI_CMDS];
#endif

#ifdef XELP_ENABLE_KEY
    XELPKeyFuncMapEntry  m_keyTable[XELP_MAX_KEY_CMDS + 1];
    int                  m_nKey = 0;
    XelpEasyKeyFn        m_easyKeyFns[XELP_MAX_KEY_CMDS];
#endif

    /* ---- Static state (single-instance limitation for easy API) --- */

#ifdef ARDUINO
    static inline Stream* s_outStream = nullptr;
#endif

#ifdef XELP_ENABLE_CLI
    static inline XelpEasyCliFn s_unknownFn = nullptr;
#endif

    /* ---- Internal helpers ----------------------------------------- */

    /** Recover XelpCLI& from XELP* using offsetof (m_x is first member). */
    static XelpCLI& _fromRaw(XELP* p)
    {
        return *reinterpret_cast<XelpCLI*>(
            reinterpret_cast<char*>(p) - offsetof(XelpCLI, m_x));
    }

#ifdef ARDUINO
    static void _streamOut(char c)
    {
        if (s_outStream) s_outStream->write(c);
    }

    static void _streamBksp()
    {
        if (s_outStream) {
            s_outStream->write('\b');
            s_outStream->write(' ');
            s_outStream->write('\b');
        }
    }
#endif

#ifdef XELP_ENABLE_CLI
    bool _addCmd(const char* name, const char* help, XelpEasyCliFn fn)
    {
        if (m_nCli >= XELP_MAX_CLI_CMDS) return false;
        m_cliTable[m_nCli] = { &_easyCliDispatch, name, help };
        m_easyCliFns[m_nCli] = fn;
        m_nCli++;
        _rebuildCliSentinel();
        return true;
    }

    void _rebuildCliSentinel()
    {
        m_cliTable[m_nCli] = {0, 0, 0};
        XELP_SET_FN_CLI(m_x, m_cliTable);
    }

    /**
     * Shared dispatcher for all Tier 3 easy CLI commands.
     * Finds the matching entry by name, parses argv, and calls the easy fn.
     * Tokens are null-terminated in place (the command buffer is writable).
     */
    static XELPRESULT _easyCliDispatch(XELP* ths, const char* args, int len)
    {
        XelpCLI& cli = _fromRaw(ths);

        /* Identify the command name (token 0). */
        XelpBuf b, tok;
        XELP_XB_INIT(b, (char*)args, len);
        if (XelpTokN(&b, 0, &tok) != XELP_S_OK)
            return XELP_E_ERR;
        int cmdLen = (int)(tok.p - tok.s);

        /* Find which table entry matches. */
        for (int i = 0; i < cli.m_nCli; i++) {
            if (cli.m_cliTable[i].mFunPtr == &_easyCliDispatch &&
                XelpStrEq(tok.s, cmdLen, cli.m_cliTable[i].mpCmd) == XELP_S_OK) {
                /* Parse argv and null-terminate each token. */
                const char* argv[XELP_MAX_EASY_ARGV];
                int argc = 0;
                XELP_XB_TOP(b);
                XelpBuf t;
                while (argc < XELP_MAX_EASY_ARGV &&
                       XelpTokN(&b, argc, &t) == XELP_S_OK) {
                    *t.p = '\0';  /* null-terminate in the writable cmd buffer */
                    argv[argc++] = t.s;
                }
                cli.m_easyCliFns[i](cli, argc, argv);
                return XELP_S_OK;
            }
        }
        return XELP_E_CMDNOTFOUND;
    }

    static XELPRESULT _unknownDispatch(XELP* ths, const char* args, int len)
    {
        if (!s_unknownFn) return XELP_S_OK;
        XelpCLI& cli = _fromRaw(ths);
        const char* argv[XELP_MAX_EASY_ARGV];
        int argc = 0;
        XelpBuf b;
        XELP_XB_INIT(b, (char*)args, len);
        XelpBuf t;
        while (argc < XELP_MAX_EASY_ARGV &&
               XelpTokN(&b, argc, &t) == XELP_S_OK) {
            *t.p = '\0';
            argv[argc++] = t.s;
        }
        s_unknownFn(cli, argc, argv);
        return XELP_S_OK;
    }
#endif /* XELP_ENABLE_CLI */

#ifdef XELP_ENABLE_KEY
    bool _addKey(XELPKEYCODE key, const char* help, XelpEasyKeyFn fn)
    {
        if (m_nKey >= XELP_MAX_KEY_CMDS) return false;
        m_keyTable[m_nKey] = { &_easyKeyDispatch, key, help };
        m_easyKeyFns[m_nKey] = fn;
        m_nKey++;
        _rebuildKeySentinel();
        return true;
    }

    void _rebuildKeySentinel()
    {
        m_keyTable[m_nKey] = {0, 0, 0};
        XELP_SET_FN_KEY(m_x, m_keyTable);
    }

    /** Shared dispatcher for all Tier 3 easy key commands. */
    static XELPRESULT _easyKeyDispatch(XELP* ths, XELPKEYCODE key)
    {
        XelpCLI& cli = _fromRaw(ths);
        for (int i = 0; i < cli.m_nKey; i++) {
            if (cli.m_keyTable[i].mFunPtr == &_easyKeyDispatch &&
                cli.m_keyTable[i].mKey == key) {
                cli.m_easyKeyFns[i](cli, key);
                return XELP_S_OK;
            }
        }
        return XELP_S_NOTFOUND;
    }
#endif /* XELP_ENABLE_KEY */
};

#endif /* XELP_ARDUINO_H */
