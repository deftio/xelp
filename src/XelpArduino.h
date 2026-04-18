/*
 * XelpArduino.h -- Header-only C++ wrapper for the xelp CLI library.
 *
 * Provides a thin XelpCLI class that wraps the raw C XELP struct.
 * Designed for Arduino but also compiles on desktop C++ (without poll()).
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

#ifdef ARDUINO
#include <Arduino.h>
#endif

class XelpCLI {
public:
    /* ---- Initialization ------------------------------------------- */

    /**
     * Initialize the xelp instance.
     * Call this in setup() after Serial.begin().
     *
     * @param aboutMsg  Null-terminated about/version string (stored by pointer).
     * @param outputFn  Function that emits one character (e.g. Serial.write).
     */
    void begin(const char* aboutMsg, void (*outputFn)(char))
    {
        XELPInit(&m_x, aboutMsg);
        XELP_SET_FN_OUT(m_x, outputFn);
    }

    /* ---- Command tables ------------------------------------------- */

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

    /* ---- Optional callbacks --------------------------------------- */

#ifdef XELP_CLI_PROMPT
    /**
     * Set the CLI prompt string (e.g. "mydev>").
     * Only effective when xelpcfg.h defines XELP_CLI_PROMPT as (ths->mpPrompt).
     * If XELP_CLI_PROMPT is a fixed string literal, this call has no effect.
     */
    void setPrompt(const char* prompt)
    {
        XELP_SET_VAL_CLI_PROMPT(m_x, prompt);
    }
#endif

#ifdef XELP_ENABLE_CLI
    /** Set the destructive-backspace handler for the CLI prompt. */
    void setBackspace(void (*fn)())
    {
        XELP_SET_FN_BKSP(m_x, fn);
    }

    /** Set the default handler for unrecognized CLI commands. */
    void setDefaultCommandHandler(XELPRESULT (*fn)(const char*, int))
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
    void setDefaultKeyHandler(XELPRESULT (*fn)(int))
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
            XELPParseKey(&m_x, c);
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
        return XELPParse(&m_x, script, XELPStrLen(script));
    }
#endif

    /** Process a single keypress through xelp's interactive parser. */
    XELPRESULT parseKey(char key)
    {
        return XELPParseKey(&m_x, key);
    }

    /** Print a null-terminated string through xelp's output function. */
    void print(const char* msg)
    {
        XELPOut(&m_x, msg, XELPStrLen(msg));
    }

#ifdef XELP_ENABLE_HELP
    /** Print the built-in help listing. */
    XELPRESULT help()
    {
        return XELPHelp(&m_x);
    }
#endif

    /* ---- Escape hatch --------------------------------------------- */

    /** Access the underlying C struct for anything the wrapper doesn't expose. */
    XELP*       raw()       { return &m_x; }
    const XELP* raw() const { return &m_x; }

private:
    XELP m_x;
};

#endif /* XELP_ARDUINO_H */
