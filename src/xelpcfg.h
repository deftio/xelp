/**

  @xelpcfg.h - header file for xelp command interpreter
 		
  @copy Copyright (C) <2011>  <M. A. Chatterjee>
  @author M A Chatterjee <deftio [at] deftio [dot] com>
  
  This file contains build-flags used to control what features are used in the xelp command library compilation.

  comment in or out to reduce libary size or set configuration such as key map.

 */

#ifndef __XELP_CONFIG_H__
#define __XELP_CONFIG_H__

#ifdef XELP_CONFIG_OVERRIDE
#include "xelp_ovr.h"  /* 8.3 filenaming convention used due to old-school compilers and filesystem support */
#else  /* use the rest of this file's conventions */


/****************************************************************************************************
 Key mappings.  Use these key mappings to switch modes at the command line  or in Key or Thru Modes.
 any valid character is allowed.  e.g. 
 #define XELPKEY_CLI ('c')  
 */

#define XELPKEY_CLI      (XELPKEY_CTP)  /* enter command mode     */
#define XELPKEY_KEY      (XELPKEY_ESC)  /* enter single key mode  */
#define XELPKEY_THR      (XELPKEY_CTT)  /* enter thru mode        */

/****************************************************************************************************
 Escape character mappings:  used to skip symbols that otherwise might be parsed such as ; or "
 */
#define XELP_CLI_ESC		('`')		  /* character used  for escaping at command line or in script */
#define XELP_QUO_ESC		('\\')		/* character used  for escaping inside quoted strings        */



/****************************************************************************************************
 Enable CLI Mode.
 definining this flag includes support for interactive command line.  Script support also requires this.

 */
#define XELP_ENABLE_CLI       1

/****************************************************************************************************
 Enable Line Editing in CLI Mode.
 When defined, provides cursor movement (left/right, Home/End), mid-line insert,
 and delete-at-cursor using only \b for terminal repositioning.
 Requires XELP_ENABLE_CLI. Adds ~800-1000 bytes on ARM Thumb.
 When not defined, CLI uses append-only input with mpfBksp callback.
 */
#define XELP_ENABLE_LINE_EDIT 1


/****************************************************************************************************
 Enable KEY Mode.
 definining this flag includes support for key mode (each immediate press is used as a command such as in a menu systems) without pressing ENTER.

 leaving undefined saves btw 200-500 bytes (target dependant)
 */
#define XELP_ENABLE_KEY 	  1


/****************************************************************************************************
 Enable THRU Mode.
 definining this flag includes support for redirecting key commands to the mpfThru function.
 Thru mode is useful for redirecting all key strokes to another peripheral such as a modem or other serial console based embedded system.

 leaving undefined saves btw 50-125 bytes (target dependant)
 */
#define XELP_ENABLE_THR 	  1	

/****************************************************************************************************
 Compile built-in help function.  
 Leaving undefined saves ~180-350 bytes. (target dependant)
 XELP_HELP_XXX_STR  are the strings used to prefix sections in the online help.  See examples or docs
 */
#define XELP_ENABLE_HELP	  1	

/****************************************************************************************************
 Help related controls 
 */
#define XELP_HELP_KEY_STR    "\nKey functions\n"        /* Help section for single-key press commands such as menus */
#define XELP_HELP_CLI_STR    "\nCLI functions\n"        /* Help string displayed before script or CLI commands      */
#define XELP_HELP_ABT_STR    (ths->mpAboutMsg)		      /* You may set to any null terminated string e.g. "My Embedded System About Message" */

/**************************************************************************************************** 
  prompt string, leave undefined (commented out) for no prompt and to save space 
  if a fixed string is provided such as "xelp>" then all instances will use this prompt.  
  if set to (ths->mpPrompt) then per-instance console prompt is set via pointer.  (see examples)  This can be usesful when different 
  instances are listening on different ports and each should have a different prompt.

  for a per-instance prompt (eg each instance of the xelp interpreter running presents a different prompt such as ser1> and ser2> ) 
  use this:
  
  #define XELP_CLI_PROMPT   (ths->mpPrompt)

  Then use the macros in Xelp.h 
  XELP_SET_VAL_CLI_PROMPT(myXelp,"yourPrompt")   //myXelp is the instance variable, message will be only for that instance.

*/
#define XELP_CLI_PROMPT		"xelp>"					


/****************************************************************************************************
  XELP_REGS_SZ is the number of callee-clobbers-all return registers per instance.
  R0: command status (written by engine after dispatch).
  R1-R3: command-specific return values (xelp engine never touches these).
  Minimum is 4. Size of each register is set by XELPREG (default is machine int).
 */
#define XELP_REGS_SZ    4

#define XELPREG int    /* can change this to a valid C type for your plaform. eg. short, long, _int64 */


/**
 Enables all features, and modes. at the expense of library size. See docs for compile sizes on tested platorms.
 */


/* #define XELP_ENABLE_FULL	  1 */


#endif	/* XELP_CONFIG_OVERRIDE (not used)*/

#endif  /* __XELP_CONFIG_H__ */
