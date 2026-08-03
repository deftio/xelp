/**

  @xelpcfg.h - header file for xelp command interpreter

  @copy Copyright (C) <2011>  <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>

  This file contains build-flags used to control what features are used in the xelp command library compilation.

  comment in or out to reduce libary size or set configuration such as key map.

 */

#ifndef __XELP_CONFIG_H__
#define __XELP_CONFIG_H__


/****************************************************************************************************
 Key mappings.  Use these key mappings to switch modes at the command line  or in Key or Thru Modes.
 any valid character is allowed.  e.g.
 #define XELPKEY_CLI ('c')
 */

#ifndef XELPKEY_CLI
#define XELPKEY_CLI      (XELPKEY_CTP)  /* enter command mode     */
#endif
#ifndef XELPKEY_KEY
#define XELPKEY_KEY      (XELPKEY_ESC)  /* enter single key mode  */
#endif
#ifndef XELPKEY_THR
#define XELPKEY_THR      (XELPKEY_CTT)  /* enter thru mode        */
#endif

/****************************************************************************************************
 Enter key detection for interactive mode (XelpParseKey).
 XELP_ENTER_LF: accept LF (\n, 0x0A) as ENTER.
 XELP_ENTER_CR: accept CR (\r, 0x0D) as ENTER.
 Enable both to accept either character — recommended for cross-platform use.
 Only affects interactive input (XelpParseKey); script parsing always uses \n.
 */
#ifndef XELP_ENTER_LF
#define XELP_ENTER_LF  1
#endif
#ifndef XELP_ENTER_CR
#define XELP_ENTER_CR  1
#endif

/****************************************************************************************************
 Escape character mappings:  used to skip symbols that otherwise might be parsed such as ; or "
 */
#ifndef XELP_CLI_ESC
#define XELP_CLI_ESC		('`')		  /* character used  for escaping at command line or in script */
#endif
#ifndef XELP_QUO_ESC
#define XELP_QUO_ESC		('\\')		/* character used  for escaping inside quoted strings        */
#endif

/****************************************************************************************************
 Quoted-string escape map: packed key-value pairs for escape expansion inside
 double-quoted arguments (_xelpBuf2Argv).  Each entry is two adjacent chars:
 the first is the character after XELP_QUO_ESC, the second is the replacement
 byte.  A lone '\0' terminates the list.  Characters not in this table pass
 through unchanged (so \\ -> \ and \" -> " work without entries).
 Override to add \r, \0, \a, etc. or set to "" to disable escape expansion.
 */
#ifndef XELP_ESC_MAP
#define XELP_ESC_MAP  "n\x0A" "t\x09"  ""
#endif



/****************************************************************************************************
 Enable CLI Mode.
 Includes interactive command line with line editing (cursor movement, insert,
 delete-at-cursor), built-in help listing, and argv tokenizer.
 Script support also requires this.
 */
#ifndef XELP_ENABLE_CLI
#define XELP_ENABLE_CLI       1
#endif

/****************************************************************************************************
 Enable Command History (UP/DOWN arrow recall).
 When defined, provides a ring buffer of previously entered commands that can
 be browsed with UP/DOWN arrows. Requires XELP_ENABLE_CLI.
 XELP_HIST_DEPTH sets the number of commands stored (default 4, overridable via
 compiler flag or xelp_ovr.h when XELP_CONFIG_OVERRIDE is defined).
 RAM cost per instance: XELP_HIST_DEPTH * XELP_CMDBUFSZ (ring) + XELP_CMDBUFSZ
 (stashed in-progress line) + 4 ints (ring indices and saved length).  At the
 defaults on a 32-bit target that is 4*64 + 64 + 16 = 336 bytes.
 The indices are int rather than char deliberately -- see the note on the
 history fields in xelp.h (issue #18).
 Code cost: ~550 bytes on ARM Thumb, ~740 on ARM32, ~910 on AVR (-Os).
 */
#ifndef XELP_ENABLE_CLI_HISTORY
#define XELP_ENABLE_CLI_HISTORY 1
#endif

/****************************************************************************************************
 Enable KEY Mode.
 definining this flag includes support for key mode (each immediate press is used as a command such as in a menu systems) without pressing ENTER.

 leaving undefined saves btw 200-500 bytes (target dependent)
 */
#ifndef XELP_ENABLE_KEY
#define XELP_ENABLE_KEY 	  1
#endif


/****************************************************************************************************
 Enable THRU Mode.
 definining this flag includes support for redirecting key commands to the mpfThru function.
 Thru mode is useful for redirecting all key strokes to another peripheral such as a modem or other serial console based embedded system.

 leaving undefined saves btw 50-125 bytes (target dependent)
 */
#ifndef XELP_ENABLE_THR
#define XELP_ENABLE_THR 	  1
#endif

/****************************************************************************************************
 Help related controls (help is always available when CLI is enabled)
 */
#ifndef XELP_HELP_KEY_STR
#define XELP_HELP_KEY_STR    "\nKey functions\n"        /* Help section for single-key press commands such as menus */
#endif
#ifndef XELP_HELP_CLI_STR
#define XELP_HELP_CLI_STR    "\nCLI functions\n"        /* Help string displayed before script or CLI commands      */
#endif
#ifndef XELP_HELP_ABT_STR
#define XELP_HELP_ABT_STR    (ths->mpAboutMsg)		      /* You may set to any null terminated string e.g. "My Embedded System About Message" */
#endif

/****************************************************************************************************
  prompt string, leave undefined (commented out) for no prompt and to save space
  if a fixed string is provided such as "xelp>" then all instances will use this prompt.
  if set to (ths->mpPrompt) then per-instance console prompt is set via pointer.  (see examples)  This can be usesful when different
  instances are listening on different ports and each should have a different prompt.

  for a per-instance prompt (eg each instance of the xelp interpreter running presents a different prompt such as ser1> and ser2> )
  use this:

  #define XELP_CLI_PROMPT   (ths->mpPrompt)

  Then use the macros in Xelp.h
  XELP_SET_VAL_CLI_PROMPT(myXelp,"yourPrompt")   (myXelp is the instance variable, message will be only for that instance.)

*/
#ifndef XELP_CLI_PROMPT
#define XELP_CLI_PROMPT		"xelp>"
#endif


/****************************************************************************************************
  XELP_REGS_SZ is the number of callee-clobbers-all return registers per instance.
  R0: command status (written by engine after dispatch).
  R1-R3: command-specific return values (xelp engine never touches these).
  Minimum is 4. Size of each register is set by XELPREG (default is machine int).
 */
#ifndef XELP_REGS_SZ
#define XELP_REGS_SZ    4
#endif

#ifndef XELPREG
#define XELPREG int    /* can change this to a valid C type for your plaform. eg. short, long, _int64 */
#endif

/****************************************************************************************************
  XELP_CMDBUFSZ is the CLI input buffer size in bytes.
  Must be at least 16. The default of 64 is suitable for most embedded targets.
  Override with -DXELP_CMDBUFSZ=128 on the compiler command line, or in xelp_ovr.h.
  RAM cost: XELP_CMDBUFSZ bytes per instance (plus history if enabled).
 */
#ifndef XELP_CMDBUFSZ
#define XELP_CMDBUFSZ       (64)
#endif

/* Migration guard: XELP_ARGV_MAX removed in 0.5.0.
   Argv capacity is now derived from XELP_ARGVBUFSZ / sizeof(pointer). */
#ifdef XELP_ARGV_MAX
#error "XELP_ARGV_MAX removed. Argv capacity is now derived from XELP_ARGVBUFSZ."
#endif

/*
  XELP_ARGVBUFSZ is the scratch buffer size for argv tokenization.
  Defaults to XELP_CMDBUFSZ.  Override to a larger value if variable expansion
  or long script lines may produce arguments longer than the CLI input buffer.
  RAM cost: XELP_ARGVBUFSZ bytes per instance (when XELP_ENABLE_CLI).
 */
#ifndef XELP_ARGVBUFSZ
#define XELP_ARGVBUFSZ      XELP_CMDBUFSZ
#endif


/**
 Enables all features, and modes. at the expense of library size. See docs for compile sizes on tested platorms.
 */


/* #define XELP_ENABLE_FULL	  1 */


/****************************************************************************************************
 Enable Script Engine.
 When defined, adds a no-malloc, ROM-able, instance-local scripting layer with variables,
 conditionals, labels/jumps, parenthesized subexpressions, script functions, math/comparison/logic
 builtins, and a breakpoint callback for safe execution. Requires XELP_ENABLE_CLI.
 */
#ifndef XELP_ENABLE_SCRIPT
#define XELP_ENABLE_SCRIPT   1
#endif

/****************************************************************************************************
 XELP_SCRIPT_ARENA_SZ is the per-instance arena buffer size in bytes for the script engine.
 The arena holds variables, result stack, call frames, and CONT records.
 Default scales with word size: 512 on 16-bit, 1024 on 32-bit, 2048 on 64-bit.
 RAM cost: XELP_SCRIPT_ARENA_SZ bytes per instance (when XELP_ENABLE_SCRIPT).
 */
#ifndef XELP_SCRIPT_ARENA_SZ
#define XELP_SCRIPT_ARENA_SZ  (sizeof(int) * 256)
#endif

/****************************************************************************************************
 XELP_CONFIG_OVERRIDE -- customize the library without modifying source files.

 To use your own configuration:
   1. Define XELP_CONFIG_OVERRIDE in your compiler flags (-DXELP_CONFIG_OVERRIDE)
   2. Create xelp_ovr.h in your include path
   3. Use #undef to disable features or #undef + #define to change values.
      Anything you don't touch keeps its default from above.

 xelp_ovr.h is included AFTER the defaults so that #undef works correctly.

 Example xelp_ovr.h -- per-instance prompt and no thru mode:

   #undef  XELP_CLI_PROMPT
   #define XELP_CLI_PROMPT   (ths->mpPrompt)

   #undef  XELP_ENABLE_THR

 See docs/build-reference.md for more examples.
 */
#ifdef XELP_CONFIG_OVERRIDE
#include "xelp_ovr.h"
#endif

/* Migration guards: these flags were removed or renamed.
   If user code still defines them, emit a helpful error. */
#ifdef XELP_ENABLE_LINE_EDIT
#error "XELP_ENABLE_LINE_EDIT removed. Line editing is now always part of XELP_ENABLE_CLI."
#endif
#ifdef XELP_ENABLE_HELP
#error "XELP_ENABLE_HELP removed. Help is now always part of XELP_ENABLE_CLI."
#endif
#ifdef XELP_ENABLE_HISTORY
#error "XELP_ENABLE_HISTORY renamed to XELP_ENABLE_CLI_HISTORY."
#endif

/* Auto-disable dependent features when their base is missing.
   CLI_HISTORY requires CLI. SCRIPT requires CLI.
   This prevents compile errors from invalid override combinations. */
#if defined(XELP_ENABLE_CLI_HISTORY) && !defined(XELP_ENABLE_CLI)
#undef XELP_ENABLE_CLI_HISTORY
#endif
#if defined(XELP_ENABLE_SCRIPT) && !defined(XELP_ENABLE_CLI)
#undef XELP_ENABLE_SCRIPT
#endif

#endif  /* __XELP_CONFIG_H__ */
