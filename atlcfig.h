#include <stdbool.h>
#ifndef __ATLAST_CFIG
#define __ATLAST_CFIG

// TODO Remove all references to EMBEDDED in io words.
// Output to a buffer is the norm.
#define EMBEDDED            // Mods for use in an embedded system.
                            // anything that results in output to stdout, or stderr goes
                            // to a global buffer.
                            //
// Define OS platform here
// #define FREERTOS
//
// LINUX should apply to any UNIX like system, such as
// MacOS X, Solaris, QNX, Cygwin ....
// #define LINUX
#define INDIVIDUALLY

#define ARRAY                 /* Array subscripting words */
#define BREAK                 /* Asynchronous break facility */
#define COMPILERW             /* Compiler-writing words */
#define CONIO                 /* Interactive console I/O */
#define DEFFIELDS             /* Definition field access for words */
#define DOUBLE                /* Double word primitives (2DUP) */
#define EVALUATE              /* The EVALUATE primitive */
// #define FILEIO                /* File I/O primitives */
// #define MATH                  /* Math functions */
#define MEMMESSAGE            /* Print message for stack/heap errors */
#define MEMSTAT
#define PROLOGUE              /* Prologue processing and auto-init */

// #define REAL                  /* Floating point numbers */
#define SHORTCUTA             /* Shortcut integer arithmetic words */
#define SHORTCUTC             /* Shortcut integer comparison */
#define STRING                /* String functions */
// #define SYSTEM                /* System command function */
#define TRACE                 /* Execution tracing */
#define WALKBACK              /* Walkback trace */
// #define WORDSUSED             /* Logging of words used and unused */
#define BANNER
// #define ATH
#define ANSI                /* Enable ANSI compatability words */
#define NVRAMRC
// #define PUBSUB              // Use the Small pub/sub system
// #define PTHREAD             // Pthreaded pubsub
// 
// #define Keyhit
// 
// Stuff added by me
// 
#define EXPORT


#endif
