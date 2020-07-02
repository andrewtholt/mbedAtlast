/*

A T L A S T

Autodesk Threaded Language Application System Toolkit

Main Interpreter and Compiler

Designed and implemented in January of 1990 by John Walker.

This program is in the public domain.

*/
#include <errno.h>
#include "io.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "atlcfig.h"
#include "atlast.h"


static int token( char **);

/*  Custom configuration.  If the tag CUSTOM has been defined (usually on
    the compiler call line), we include the file "atlcfig.h", which may
    then define INDIVIDUALLY and select the subpackages needed for its
    application.  */

#ifdef CUSTOM
#include "atlcfig.h"
#endif

/*  Subpackage configuration.  If INDIVIDUALLY is defined, the inclusion
    of subpackages is based on whether their compile-time tags are
    defined.  Otherwise, we automatically enable all the subpackages.  */

#ifndef INDIVIDUALLY
#define ARRAY			      /* Array subscripting words */
#define BREAK			      /* Asynchronous break facility */
#define COMPILERW		      /* Compiler-writing words */
#define CONIO			      /* Interactive console I/O */
#define DEFFIELDS		      /* Definition field access for words */
#define DOUBLE			      /* Double word primitives (2DUP) */
#define EVALUATE		      /* The EVALUATE primitive */
#define FILEIO			      /* File I/O primitives */
#define MATH			      /* Math functions */
#define MEMMESSAGE		      /* Print message for stack/heap errors */
#define PROLOGUE		      /* Prologue processing and auto-init */
#define REAL			      /* Floating point numbers */
#define SHORTCUTA		      /* Shortcut integer arithmetic words */
#define SHORTCUTC		      /* Shortcut integer comparison */
#define STRING			      /* String functions */
#define SYSTEM			      /* System command function */
#ifndef NOMEMCHECK
#define TRACE			      /* Execution tracing */
#define WALKBACK		      /* Walkback trace */
#define WORDSUSED		      /* Logging of words used and unused */
#endif /* NOMEMCHECK */
#endif /* !INDIVIDUALLY */


#include "atldef.h"

#ifdef MATH
#include <math.h>
#endif

/* LINTLIBRARY */

/* Implicit functions (work for all numeric types). */

#ifdef abs
#undef abs
#endif
#define abs(x)	 ((x) < 0    ? -(x) : (x))
#define max(a,b) ((a) >  (b) ?	(a) : (b))
#define min(a,b) ((a) <= (b) ?	(a) : (b))

// ATH runflag, setup at the end of atl_init
// dictword *rf;

/*  Globals imported  */

/*  Data types	*/

/* typedef enum {False = 0, True = 1} Boolean;
*
*/

#define EOS     '\0'                  /* End of string characters */

// #define V	(void)		      /* Force result to void */

#define Truth	-1L		      /* Stack value for truth */
#define Falsity 0L		      /* Stack value for falsity */

/* Utility definition to get an  array's  element  count  (at  compile
time).   For  example:

int  arr[] = {1,2,3,4,5};
...
printf("%d", ELEMENTS(arr));

would print a five.  ELEMENTS("abc") can also be used to  tell  how
many  bytes are in a string constant INCLUDING THE TRAILING NULL. */

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

/*  Globals visible to calling programs  */

atl_int atl_stklen = 100;	      /* Evaluation stack length */
atl_int atl_rstklen = 100;	      /* Return stack length */
// atl_int atl_heaplen = 1000;	      /* Heap length */
atl_int atl_heaplen = 2000;	      /* Heap length */
atl_int atl_ltempstr = 256;	      /* Temporary string buffer length */
atl_int atl_ntempstr = 4;	      /* Number of temporary string buffers */

atl_int atl_trace = Falsity;	      /* Tracing if true */
atl_int atl_walkback = Truth;	      /* Walkback enabled if true */
atl_int atl_comment = Falsity;	      /* Currently ignoring a comment */
atl_int atl_redef = Truth;	      /* Allow redefinition without issuing
                                    the "not unique" message. */
atl_int atl_errline = 0;	      /* Line where last atl_load failed */

// #ifdef ATH
atl_int ath_safe_memory = Truth;
// #endif

/*  Local variables  */

/* The evaluation stack */

Exported stackitem *stack = NULL;     /* Evaluation stack */
Exported stackitem *stk;	      /* Stack pointer */
Exported stackitem *stackbot;	      /* Stack bottom */
Exported stackitem *stacktop;	      /* Stack top */

/* The return stack */

Exported dictword ***rstack = NULL;   /* Return stack */
Exported dictword ***rstk;	      /* Return stack pointer */
Exported dictword ***rstackbot;       /* Return stack bottom */
Exported dictword ***rstacktop;       /* Return stack top */

/* The heap */

Exported stackitem *heap = NULL;      /* Allocation heap */
Exported stackitem *hptr;	      /* Heap allocation pointer */
Exported stackitem *heapbot;	      /* Bottom of heap (temp string buffer) */
Exported stackitem *heaptop;	      /* Top of heap */

/* The dictionary */

Exported dictword *dict = NULL;       /* Dictionary chain head */
Exported dictword *dictprot = NULL;   /* First protected item in dictionary */

/* The temporary string buffers */

Exported char **strbuf = NULL;	      /* Table of pointers to temp strings */
Exported int cstrbuf = 0;	      /* Current temp string */

/* The walkback trace stack */

#ifdef WALKBACK
static dictword **wback = NULL;       /* Walkback trace buffer */
static dictword **wbptr;	      /* Walkback trace pointer */
#endif /* WALKBACK */

#ifdef MEMSTAT
stackitem *stackmax;	      /* Stack maximum excursion */
dictword ***rstackmax;       /* Return stack maximum excursion */
stackitem *heapmax;	      /* Heap maximum excursion */
#endif


static char tokbuf[128];	      /* Token buffer */
static char *instream = NULL;	      /* Current input stream line */
static long tokint;		      /* Scanned integer */
#ifdef REAL
static atl_real tokreal;	      /* Scanned real number */
#ifdef ALIGNMENT
Exported atl_real rbuf0, rbuf1, rbuf2; /* Real temporary buffers */
#endif
#endif
static long base = 10;		      /* Number base */
Exported dictword **ip = NULL;	      /* Instruction pointer */
Exported dictword *curword = NULL;    /* Current word being executed */
static int evalstat = ATL_SNORM;      /* Evaluator status */
static Boolean defpend = False;       /* Token definition pending */
static Boolean forgetpend = False;    /* Forget pending */
static Boolean tickpend = False;      /* Take address of next word */
static Boolean ctickpend = False;     /* Compile-time tick ['] pending */
static Boolean cbrackpend = False;    /* [COMPILE] pending */
Exported dictword *createword = NULL; /* Address of word pending creation */
static Boolean stringlit = False;     /* String literal anticipated */
#ifdef BREAK
static Boolean broken = False;	      /* Asynchronous break received */
#endif
prim P_con();

#ifdef COPYRIGHT
#ifndef HIGHC
#ifndef lint
static
#endif
#endif
char copyright[] = "ATLAST: This program is in the public domain.";
#endif

/* The following static cells save the compile addresses of words
generated by the compiler.  They are looked up immediately after
the dictionary is created.  This makes the compiler much faster
since it doesn't have to look up internally-reference words, and,
far more importantly, keeps it from being spoofed if a user redefines
one of the words generated by the compiler.	*/

static stackitem s_exit, s_lit, s_strlit, s_dotparen,
                s_qbranch, s_branch, s_xdo, s_xqdo, s_xloop,
                s_pxloop, s_abortq;
#ifdef REAL
static stackitem s_flit;
#endif
/*  Forward functions  */

// STATIC void exword(), trouble();
#ifndef NOMEMCHECK
STATIC void notcomp(), divzero();
#endif
#ifdef WALKBACK
STATIC void pwalkback();
#endif

void testing() {
}

prim ATH_Token() {
    V token(&instream);

    V strcpy(strbuf[cstrbuf], tokbuf);
    Push = (stackitem) strbuf[cstrbuf];
}

// #ifdef ATH
void ATH_Features() {

#ifdef ARRAY
    atlastTxString((char *)"\n    ARRAY\r\n");
#else
    atlastTxString((char *)"\nNOT ARRAY\r\n");
#endif

#ifdef EMBEDDED
    atlastTxString((char *)"    EMBEDDED\r\n");
#else
    atlastTxString((char *)"NOT EMBEDDED\r\n");
#endif

#ifdef BREAK
    atlastTxString((char *)"    BREAK\r\n");
#else
    atlastTxString((char *)"NOT BREAK\r\n");
#endif
//
#ifdef COMPILERW
    atlastTxString((char *)"    COMPILERW\r\n");
#else
    atlastTxString((char *)"NOT COMPILERW\r\n");
#endif

#ifdef CONIO
    atlastTxString((char *)"    CONIO\r\n");
#else
    atlastTxString((char *)"NOT CONIO\r\n");
#endif

#ifdef DEFFIELDS
    atlastTxString((char *)"    DEFFIELDS\r\n");
#else
    atlastTxString((char *)"NOT DEFFIELDS\r\n");
#endif

#ifdef DOUBLE
    atlastTxString((char *)"    DOUBLE\r\n");
#else
    atlastTxString((char *)"NOT DOUBLE\r\n");
#endif

#ifdef EVALUATE
    atlastTxString((char *)"    EVALUATE\r\n");
#else
    atlastTxString((char *)"NOT EVALUATE\r\n");
#endif

#ifdef FILEIO
    atlastTxString((char *)"    FILEIO\r\n");
#else
    atlastTxString((char *)"NOT FILEIO\r\n");
#endif

#ifdef MATH
    atlastTxString((char *)"    MATH\r\n");
#else
    atlastTxString((char *)"NOT MATH\r\n");
#endif

#ifdef MEMMESSAGE
    atlastTxString((char *)"    MEMMESSAGE\r\n");
#else
    atlastTxString((char *)"NOT MEMMESSAGE\r\n");
#endif

#ifdef PROLOGUE
    atlastTxString((char *)"    PROLOGUE\r\n");
#else
    atlastTxString((char *)"NOT PROLOGUE\r\n");
#endif

#ifdef REAL
    atlastTxString((char *)"    REAL\r\n");
#else
    atlastTxString((char *)"NOT REAL\r\n");
#endif

#ifdef SHORTCUTA
    atlastTxString((char *)"    SHORTCUTA\r\n");
#else
    atlastTxString((char *)"NOT SHORTCUTA\r\n");
#endif

#ifdef SHORTCUTC
    atlastTxString((char *)"    SHORTCUTC\r\n");
#else
    atlastTxString((char *)"NOT SHORTCUTC\r\n");
#endif

#ifdef STRING
    atlastTxString((char *)"    STRING\r\n");
#else
    atlastTxString((char *)"NOT STRING\r\n");
#endif

#ifdef SYSTEM
    atlastTxString((char *)"    SYSTEM\r\n");
#else
    atlastTxString((char *)"NOT SYSTEM\r\n");
#endif

#ifdef TRACE
    atlastTxString((char *)"    TRACE\r\n");
#else
    atlastTxString((char *)"NOT TRACE\r\n");
#endif

#ifdef WALKBACK
    atlastTxString((char *)"    WALKBACK\r\n");
#else
    atlastTxString((char *)"NOT WALKBACK\r\n");
#endif

#ifdef WORDSUSED
    atlastTxString((char *)"    WORDSUSED\r\n");
#else
    atlastTxString((char *)"NOT WORDSUSED\r\n");
#endif

// ------------------
#ifdef ATH
    atlastTxString((char *)"\r\n    ATH CUSTOM\r\n");
#else
    atlastTxString((char *)"\r\nNOT ATH CUSTOM\r\n");
#endif
// ------------------
#ifdef PUBSUB
    atlastTxString((char *)"    PUBSUB\r\n");
#else
    atlastTxString((char *)"NOT PUBSUB\r\n");
#endif
//
#ifdef MQTT
    atlastTxString((char *)"    MQTT\r\n");
#else
    atlastTxString((char *)"NOT MQTT\r\n");
#endif
//
// ------------------

#ifdef FREERTOS
    atlastTxString((char *)"    FREERTOS\r\n");
#else
    atlastTxString((char *)"NOT FREERTOS\r\n");
#endif

#ifdef ANSI
    atlastTxString((char *)"    ANSI\r\n");
#else
    atlastTxString((char *)"NOT ANSI\r\n");
#endif
#ifdef LINUX
    atlastTxString((char *)"    LINUX\r\n");
#else
    atlastTxString((char *)"NOT LINUX\r\n");
#endif
}


#ifdef ATH

prim ATH_Instream() {
    Push=(stackitem)instream;
}


prim ATH_pwd() {
    Sl(2); // pointer to a memory area large enough to hold the biggest path
#ifdef LINUX
    char *p;

    p=getcwd( (char *)S1, (size_t)S0);
    Pop;

    if(!p) {
        S0=true;
    } else {
        S0=false;
    }
#else
    Pop;
    S0=true;

#endif
}

prim ATH_cd() {
    Sl(1);
#ifdef LINUX
    int rc=0;
    rc=chdir((char *)S0);
    if(rc < 0) {
        S0=true;
    } else {
        S0=false;
    }
#else
    S0=true;
#endif
}
prim ATH_errno() {
        Sl(0);
        So(1);
        Push=errno;

        errno=0;
}

prim ATH_help() {
        Sl(0);
        So(0);

}

prim ATH_banner() {
                uint8_t msgBuffer[80];
                memset(msgBuffer,0,80);

                strcpy ((char *)msgBuffer, (char *)"\r\nBased on ATLAST 1.2 (2007-10-07)\n");
#ifdef MBED
        atlastTxString((char *)msgBuffer);
#endif

                strcpy((char *)msgBuffer, (char *)"\rThe original version of this program is in the public domain.\n");
#ifdef MBED
        atlastTxString((char *)msgBuffer);
#endif
                strcpy((char *)msgBuffer,"\r\nCompiled: " );
                strcat((char *)msgBuffer, __DATE__);
                strcat((char *)msgBuffer," ");
                strcat((char *)msgBuffer, __TIME__);
                strcat((char *)msgBuffer,"\r\n");

#ifdef MBED
        atlastTxString((char *)msgBuffer);
#endif
}

prim ATH_perror() {
    char *msg;

    Sl(1);
    So(0);

    atlastTxString( (char  *)S0) ;
    atlastTxString((char *)":") ;
    msg = strerror(errno);
    atlastTxString( (char *)msg) ;
    P_cr();
}

prim RT_dir() {
    Sl(1);
    So(0);
#ifdef YAFFS
    char *name = S0;							// we expect 1 item on the stack (full file name)

    directory(name);

    S0 = false;
#else
    S0=true;
#endif
}

prim RT_touch() {
    Sl(1);										// we expect 1 item on the stack (full file name)
    So(0);										// we aren't adding anything to the stack (we are replacing name with success/fail
#ifdef YAFFS
    char *name = S0;
    int rc;

    rc = create_random_file(name, 0, 0, 0);		// create returns -1 for fail, 0 for success

    if (rc < 0)
    {
        S0 = true;								// fail
    }
    else
    {
        S0 = false;								// success
    }
#else
    S0 = true;									// fail
#endif
}

prim RT_mkfile() {
    Sl(2);										// we expect 2 item on the stack (full file name and length)
    So(0);										// we aren't adding anything to the stack (we are replacing 2 items with just 1 - success/fail)
#ifdef YAFFS
    char *name = S1;
    int length = S0;
    int rc;

    Pop;										// trash 1 item

    rc = create_random_file(name, length, 0, 0);	// create returns -1 for fail, 0 for success

        if (rc < 0)
    {
        S0 = true;								// fail
    }
    else
    {
        S0 = false;								// success
    }
#else
    S0 = true;									// fail
#endif
}

prim RT_crcfile() {
    Sl(1);										// we expect 1 item on the stack (full file name)
    So(1);										// we are possibly adding 1 item to the stack (we are replacing 1 item with 1 or 2)
#ifdef YAFFS
    char *name = S0;
    int crc;
    int rc;

    Pop;										// trash the name

    rc = crc_file(name, (uint32_t *)&crc);		// crc returns -1 for fail, 0 for success

    if (rc < 0)									// failed
    {
        Push = true;							// return fail
    }
    else
    {
        Push = crc;								// return crc ...
        Push = false;							// ... and success
    }
#else
    S0 = true;									// fail
#endif
}

prim RT_test() {
    Sl(0);										// we expect 0 items on the stack
    So(1);										// we are responding with a result
#ifdef YAFFS
    yaffs_test();

        Push = false;								// success
#else
    S0 = true;									// fail
#endif
}

prim ATH_ms() {
    Sl(1);
#ifdef LINUX
    // TODO: Fix this.
#ifndef UCLINUX
    usleep((useconds_t)S0 * 1000);
#endif
#endif

#ifdef FREERTOS
    vTaskDelay(pdMS_TO_TICKS((uint32_t)S0));
#endif
#ifdef MBED
#endif
    Pop;
}
prim ATH_qlinux() {
#ifdef LINUX
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qfreertos() {
#ifdef FREERTOS
    Push=-1;
#else
    Push=0;
#endif
}

prim ATH_qfileio() {
    So(1);

#ifdef FILEIO
    Push=-1;
#else
    Push=0;
#endif
}
prim ATH_memsafe() {
    Sl(1);
    ath_safe_memory = (S0 == 0) ? Falsity : Truth;
    Pop;
}

prim ATH_qmemsafe() {
    So(1);
    Push = ath_safe_memory;
}
// TODO Tidy this up.  Make non embedded version still copy to outBuffer,
// but change output method.
//
void displayLineHex(uint8_t *a) {
        int i;
    char buffer[8];

        for(i=0;i<16;i++) {
        sprintf(buffer," %02x",*(a++));
        atlastTxString(buffer);
        }
}

void displayLineAscii(uint8_t *a) {
        int i;

    atlastTxByte(':');

        for(i=0;i<16;i++) {
                if( (*a < 0x20 ) || (*a > 0x80 )) {
            atlastTxByte('.');
//			printf(".");
                        a++;
                } else {
            atlastTxByte(*(a++));
//			printf("%c",*(a++));
                }
        }
    atlastTxByte('\r');
    atlastTxByte('\n');
}

prim ATH_on() {
    Push=-1;
}

prim ATH_off() {
    Push=0;
}
#ifdef SOCKETS
prim athConnect() {
    char           *hostName;
    int             len, port;
    int             tmp;
    int             sock1;
    int             exitStatus = 0;
    struct sockaddr_in serv_addr;
    struct hostent *hp;
    int rc;

    struct addrinfo *result = NULL;
    struct addrinfo hint;

    char portNumber[8];

    Sl(2);
    So(1);

    memset(&hint, 0 , sizeof(hint));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    port = S0;
    hostName = S1;
    Pop2;

    sprintf(portNumber,"%d",port);

    rc = getaddrinfo(hostName, portNumber, &hint, &result);

        if( 0 == rc ) {
        sock1 = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if(sock1 < 0) {
            exitStatus = -1;
        } else {
            tmp = connect(sock1, result->ai_addr, result->ai_addrlen );
            if (tmp < 0) {
                exitStatus = -1;
            }
        }
    }
    if (exitStatus == 0) {
        Push=sock1;
    }
    Push=exitStatus;

}

prim athClose() {
    int             sock;

    Sl(1);
    So(0);

    sock = S0;
    Pop;
    close(sock);
}

prim athSend() {
    char           *buffer;
    int             len;
    int flag=0;
    int             sock2;
    int             status;

    Sl(3);
    So(2);

    sock2 = S0;
    len = S1;
    buffer = (char *)S2;
    Npop(3);

    status = send(sock2, buffer, len, 0);
    Push=status;

    flag = ( status < 0 );
    Push = flag;
}

prim athRecv() {
    int             n;
    int             sock2;
    int             len;
    int flag = 0;
    char           *msg;

    Sl(3);
    So(1);

    sock2 = S0;
    len = S1;
    msg =(char *)S2;
    Npop(3);
    //
    // Wipe the buffer first
    //
    bzero(msg,len);
    n = recv(sock2, msg, len, 0);
    Push = n;
}
#endif
//
// Add \n to a string
// Stack : pointer -- pointer
//
prim athAddEOL() {
    char *ptr = S0;
    strcat(ptr,"\n");
}
/*
//
// name sock -- str:value
//
prim athCmdGet() {
    Sl(2);
    So(1);

    char cmd[255];
    char in[255];
    int sock = (int)S0;
    char *name = (char *)S1;
    Pop2;

    bzero(cmd,255);
    bzero(in,255);

    sprintf(cmd,"GET %s\n",name);

    int status = send(sock, cmd, strlen(cmd), 0);
    status = recv(sock, in, 255, 0);

    if(!strcmp(in,"ON\n")) {
        Push=-1;
    } else {
        Push=0;
    }
}
//
// Stack: str:name str:value sock
//
prim athCmdSet() {
    Sl(3);
    Sl(1);

    int sock = (int)S0;
    char *def=S1;
    char *name= S2;

    Npop(3);

    char cmd[255];
    char in[255];

    bzero(cmd,255);
    bzero(in,255);

    sprintf(cmd,"SET %s %s\n",name, def);

    int status = send(sock, cmd, strlen(cmd), 0);
    status = recv(sock, in, 255, 0);

    Push = status;
}
*/
prim ATH_dump() {
    Sl(2); // address len

    int length = S0;
    uint8_t *address = (uint8_t *) S1;
    int lines=length/16;

    if(lines ==0 ) {
        lines=1;
    }
#ifdef MBED
    atlastTxByte('\r');
    atlastTxByte('\n');
#endif

    int i=0;
    char buffer[16];
    for( i = 0; i<length;i+=16) {
//        printf("%08x:", (uintptr_t)address);
    #ifdef MBED
        sprintf(buffer,"%08x:", (uintptr_t)address);
        atlastTxString((char *)buffer);
    #endif
        displayLineHex( address );
        displayLineAscii( address );
        address +=16;
    }

    Pop2;
}

prim ATH_erase() {
    Sl(2);

    int length = S0;
    uint8_t *address = (uint8_t *) S1;

    memset(address,0,length);

    Pop2;
}

prim ATH_fill() {
    Sl(3);

    uint8_t d = S0;
    int length = S1;
    uint8_t *address = (uint8_t *) S2 ;

    memset(address,d,length);

    Pop2;
    Pop;
}

prim ATH_hex() {
    base = 16;
}

prim ATH_dec() {
    base = 10;
}

prim ATH_bye() {
//    *((int *) atl_body(rf)) = 0;
#ifdef FREETOS
        atl_eval("0 DBG_RUN !");
#endif

#if defined(LINUX) || defined(DARWIN)
        exit(0);
#endif
}
#endif // ATH

int8_t readLineFromArray(uint8_t *src, uint8_t *dest) {
    uint8_t ch;
    int i;
    int8_t len=0;
    static uint32_t offset=0;

    for(i=0;i<MAX_LINE;i++) {
        ch=*(src+(offset++));
        if ( ch == 0 ) {
            len=-1;
            *dest=0;
            break;
        } else if( ch == '\n' || ch == '\r' ) {
            *dest=0;
            break;
        } else {
            *(dest++)=ch;
            len=i;
        }
    }
    return len;
}
#ifdef ANSI
prim ANSI_cell() {
    So(1);
    Push = sizeof(int);
}

prim ANSI_cells() {
    Sl(1);
    S0 = S0 * sizeof(int);
}

prim ANSI_cellplus() {
    Sl(1);
    S0 = S0 + sizeof(int);
}

prim ANSI_chars() {
        Sl(1);

        S0 = sizeof(uint8_t) * S0;

}

prim ANSI_allocate() {
        void *ptr=NULL;
        Sl(1);
        So(2);

        ptr=malloc(S0);
        if(ptr != NULL) {
                memset(ptr,0,S0);
                Pop;
                Push=(stackitem)ptr;
                Push=0;
        } else {
                Push=0;
                Push=-1;
        }
}

prim ANSI_free() {
        Sl(1);
    So(1);

        if(S0 != 0 ) {
                free((void *)S0);
        }
//	Push=0;
    S0 = 0;
}

#endif // ANSI


#ifdef PUBSUB
/*
* Name: message@
* Stack: <from> <timeout> <message ptr>
*
* Description:
*  Receive a message, waiting for a specified number of milli seconds, and
*  place the recieved message at ptr.
*/
prim FR_getMessage() {
#ifdef FREERTOS
//	extern struct taskData *task[LAST_TASK];
        BaseType_t rc;
        uint32_t timeout;
        volatile QueueHandle_t qh;
    struct cmdMessage *out;
    bool failFlag=true;

        Sl(3);
        So(1);

    out = (struct cmdMessage *)S0;
        timeout=S1;
    qh=(QueueHandle_t)S2;

    rc = xQueueReceive( qh, out, pdMS_TO_TICKS(timeout));

        if( rc != pdPASS ) {
                memset( out, 0, sizeof(struct cmdMessage) );
            failFlag=true;
        } else {
            failFlag=false;
        }
    Pop2;

        S0 = (stackitem)failFlag;
#endif

#ifdef LINUX
        uint32_t timeout;
        char *from;
        struct mq_attr attr;
        int rc=0;

        struct cmdMessage *out;
        Sl(3);
        So(1);

        out = S0;
        timeout=S1;
        from = S2; // mq name.  TODO change to a file descriptor

        mqd_t mq=mq_open(from, O_RDONLY);
        if((mqd_t)-1 == mq) {
                perror("MESSAGE@");
        }
        rc = mq_getattr(mq,&attr);

        int len = mq_receive(mq,out,attr.mq_msgsize, NULL);
        Npop(3);

        if( len <0)  {
                Push=-1;
        } else {
                Push=0;
        }

#endif

}
// Set fields in a message.
//
// <address> <value> -- <address>
prim FR_setSender() {
    Sl(2);
    struct cmdMessage *msg;

#ifdef FREERTOS
    QueueHandle_t value;
    value=(QueueHandle_t)S0;

    msg = (struct cmdMessage *)S1;
    msg->sender = value;
#endif

#ifdef LINUX
    char *value;
    value = (char *)S0;
    strncpy(msg->sender, value,SENDER_SIZE );

#endif


    Pop;
}
// Get sender
// <address> -- <sender>
prim FR_getSender() {
    struct cmdMessage *msg;

    msg = (struct cmdMessage *)S0;
    S0 = (stackitem)msg->sender;

}
// <struct address> <string> -- <struct address>
//
prim FR_setCmd() {
        Sl(2);
        So(1);

    struct cmdMessage *msg;
    char *cmd;

    cmd = (char *)S0;
    msg = (struct cmdMessage *)S1;

    Pop;

    strncpy(msg->payload.message.cmd, cmd, MAX_CMD);
}

prim FR_getCmd() {
        Sl(1);
        So(1);

    struct cmdMessage *msg;
    char *cmd;

    msg = (struct cmdMessage *)S0;

    S0 = msg->payload.message.cmd;
}

prim FR_setKey() {
        Sl(2);
        So(1);

    struct cmdMessage *msg;
    char *key;

    key = (char *)S0;
    msg = (struct cmdMessage *)S1;

    Pop;

    strncpy(msg->payload.message.key, key, MAX_KEY);
}

prim FR_getKey() {
        Sl(1);
        So(1);

    struct cmdMessage *msg;

    msg = (struct cmdMessage *)S0;

    S0=msg->payload.message.key;
}

prim FR_setValue() {
    struct cmdMessage *msg;
    char *value;

    value = (char *)S0;
    msg = (struct cmdMessage *)S1;

    Pop;

    strncpy(msg->payload.message.value, value, MAX_VALUE);
}

prim FR_getValue() {
    struct cmdMessage *msg;

    msg = (struct cmdMessage *)S0;
    S0=msg->payload.message.value;
}


prim FR_setFieldCnt() {
        struct cmdMessage *msg;
        uint8_t fields;

        fields = (uint8_t)S0;
        msg = (struct cmdMessage *)S1;


        msg->payload.message.fields = fields;

        Pop;

}

prim FR_getFieldCnt() {
        Sl(1);
        So(1);

        struct cmdMessage *msg;

        msg=(struct cmdMessage *)S0;

        S0=(stackitem)msg->payload.message.fields;
}

//
// populate a GET message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgGet() {
    struct cmdMessage *msg;
    char *key;

    Sl(3);
#ifdef FREERTOS
    QueueHandle_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
    msg=(struct cmdMessage *)S2;

#ifdef FREERTOS
    sender=(QueueHandle_t) S1;
#endif

    mkMsg(sender, msg, "GET", key, NULL);
    Pop;
    Pop2;
}
//
// populate a SET message
// Stack <msg pointer> <key> <value> --
//
prim FR_mkmsgSet() {
    struct cmdMessage *msg;
    char *key;
    char *value;

    Sl(3);

#ifdef FREERTOS
    QueueHandle_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    value=(char *)S0;
    key=(char *)S1;
    msg=(struct cmdMessage *)S2;

#ifdef FREERTOS
    sender=NULL;
#endif

    mkMsg(sender, msg, "SET", key, value);
    Pop;
    Pop2;
}


//
// populate a SUB message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgSub() {
    struct cmdMessage *msg;
    char *key;

#ifdef FREERTOS
    QueueHandle_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
#ifdef LINUX
    strncpy(msg->sender,(char *)S1, SENDER_SIZE);
#endif

#ifdef FREERTOS
    sender=(QueueHandle_t) S1;
#endif
    msg=(struct cmdMessage *)S2;

    mkMsg(sender, msg, "SUB", key, NULL);
    Pop;
    Pop2;
}
//
// populate an UNSUB message
// Stack <msg pointer> <sender> <key> --
//
prim FR_mkmsgUnsub() {
    struct cmdMessage *msg;
    char *key;

#ifdef FREERTOS
    QueueHandle_t sender;
#endif

#ifdef LINUX
    char *sender;
#endif

    key=(char *)S0;
#ifdef LINUX
    strncpy(msg->sender,(char *)S1, SENDER_SIZE);
#endif

#ifdef FREERTOS
    sender=(QueueHandle_t) S1;
#endif
    msg=(struct cmdMessage *)S2;

    mkMsg(sender, msg, "UNSUB", key, NULL);
    Pop;
    Pop2;
}

// TODO FreeRTOS Only and temporary at that
// Ultimately these will become open close etc
//
#ifdef FREERTOS
// <msg ptr> <sender> filename mode --
//
prim FR_mkmsgOpen() {
        Sl(4);
        So(0);

        struct cmdMessage *out = S3;
    QueueHandle_t sender = S2;
        char *fname = S1;
        char *mode = S0;

        memset(out,0,sizeof(struct cmdMessage));

        strcpy(out->payload.message.cmd, "OPEN");
        strcpy(out->payload.message.key, fname);
        strcpy(out->payload.message.value, mode);

        out->payload.message.fields = 3;
        out->sender = sender;

        Npop(4);

}
#endif

prim FR_putMessage() {
        struct cmdMessage *out;
        Sl(2);
        So(1);
#ifdef FREERTOS
        volatile QueueHandle_t dest;
        BaseType_t status = errQUEUE_FULL;
        bool rc=true;;

        Sl(2);
        So(1);

        dest = (QueueHandle_t ) S1;
        out = (struct cmdMessage *)S0;

        if ( (dest != NULL) && (out != NULL)) {
                status = xQueueSendToBack(dest,out, osWaitForever);

                if( rc == pdPASS) {
                        rc=false;
                } else {
                        rc=true;
                }
        }
        Pop2;
//	S0=rc;

#endif
#ifdef LINUX
        char *dest = (char *)S1;
    int rc=0;

        out = (struct cmdMessage *)S0;

        mqd_t mq=mq_open(dest,O_WRONLY);
        if ((mqd_t) -1 == mq) {
                perror("MESSAGE! mq_open");
                exit(2);
        }
    rc = mq_send(mq,out,sizeof(struct cmdMessage),(size_t)NULL);

    mq_close( mq ) ;
    Pop2;
#endif
        Push=rc;
}

prim FR_mkdb() {
        So(1);

        struct Small *db = newSmall();

        Push = (stackitem) db;

}

prim FR_publish() {
        char *name;
        struct Small *db;
        bool rc;

        Sl(2);

        name=(char *)S0;
        db=(struct Small *)S1;

        rc=dbPublish(db,name);

        Pop2;

        Push=rc;
}

prim FR_subCount() {
    int32_t cnt;
    char *key;
    struct Small *db;

    Sl(2);
    So(1);

    key=(char *)S0;
    db=(struct Small *)S1;

    cnt=(int32_t)getSubCount(db, key);
    Pop;
    S0=(int32_t)cnt;
}



prim FR_displayRecord() {
    struct nlist *rec;
    Sl(1);
    char localBuffer[80];

    rec=(struct nlist *)S0;
    sprintf(localBuffer,"Name      : %s\n", nlistGetName(rec));
#ifdef MBED
    atlastTxString(localBuffer);
#endif

    sprintf(localBuffer,"Value     : %s\n", (char *)nlistGetDef(rec));
#ifdef MBED
    atlastTxString(localBuffer);
#endif

    sprintf(localBuffer, "Published :" );

    if(dbAmIPublished(rec)) {
        strcat(localBuffer,"True\n");
    } else {
        strcat(localBuffer,"False\n");
    }
#ifdef MBED
    atlastTxString(localBuffer);
#endif
    Pop;
}

prim FR_addRecord() {
        struct Small *db;
        char *n;
        char *v;
        bool rc;

        Sl(3);
        So(1);

        v=(char *)S0;
        n=(char *)S1;
        db=(struct Small *)S2;

        rc = addRecord(db,n,v);

        Npop(3);

        Push=rc;

}
//    char *dbLookup(struct Small *db, const char *n);
// Stack: db <key> -- <nlist>
prim FR_lookupRecord() {
        struct Small *db;
        char *key;
        struct nlist *rec;

        Sl(2);
        So(1);

        key=(char *)S0;
        db=(struct Small *)S1;
    Pop2;

        rec = dbLookupRec(db,key);

    Push = (stackitem)rec;

    if( rec == NULL ) {
        Push = true;
    } else {
        Push = false;
    }
}

prim FR_lookup() {

        struct Small *db;
        char *key;
        char *value;

        Sl(2);
        So(1);

        key=(char *)S0;
        db=(struct Small *)S1;

        value = dbLookup(db,key);
        Pop2;

        Push = (stackitem)value;

        if( value == NULL) {
                Push = true;
        } else {
                Push = false;
        }

}
#endif

/*  ALLOC  --  Allocate memory and error upon exhaustion.  */

static char *alloc(unsigned int size) {
    char *cp = (char *)malloc(size);

    if (cp == NULL) {
        char buffer[80];

        sprintf(buffer,"\n\nOut of memory!  %u bytes requested.\n", size); // EMBEDDED

#ifdef MBED
        atlastTxString(buffer);
#endif
        abort();
    }
    return cp;
}

/*  UCASE  --  Force letters in string to upper case.  */

static void ucase(char *c) {
    char ch;

    while ((ch = *c) != EOS) {
        if (islower(ch))
            *c = toupper(ch);
        c++;
    }
}

/*  TOKEN  --  Scan a token and return its type.  */

static int token( char **cp) {
    char *sp = *cp;

    while (True) {
        char *tp = tokbuf;
        unsigned int tl = 0;
        Boolean istring = False, rstring = False;

        if (atl_comment) {
            while (*sp != ')') {
                if (*sp == EOS) {
                    *cp = sp;
                    return TokNull;
                }
                sp++;
            }
            sp++;
            atl_comment = Falsity;
        }

        while (isspace(*sp))		  /* Skip leading blanks */
            sp++;

        if (*sp == '"') {                 /* Is this a string ? */

            /* Assemble string token. */

            sp++;
            while (True) {
                char c = *sp++;

                if (c == '"') {
                    sp++;
                    *tp++ = EOS;
                    break;
                } else if (c == EOS) {
                    rstring = True;
                    *tp++ = EOS;
                    break;
                }
                if (c == '\\') {
                    c = *sp++;
                    if (c == EOS) {
                        rstring = True;
                        break;
                    }
                    switch (c) {
                        case 'b':
                            c = '\b';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        default:
                            break;
                    }
                }
                if (tl < (sizeof tokbuf) - 1) {
                    *tp++ = c;
                    tl++;
                } else {
                    rstring = True;
                }
            }
            istring = True;
        } else {

            /* Scan the next raw token */

            while (True) {
                char c = *sp++;

                if (c == EOS || isspace(c)) {
                    *tp++ = EOS;
                    break;
                }
                if (tl < (sizeof tokbuf) - 1) {
                    *tp++ = c;
                    tl++;
                }
            }
        }
        *cp = --sp;			  /* Store end of scan pointer */

        if (istring) {
            if (rstring) {
#ifdef MEMMESSAGE
#ifdef MBED
                char buffer[80];
                sprintf(buffer,"\nRunaway string: %s\n", tokbuf); // EMBEDDED
#endif
#endif
                evalstat = ATL_RUNSTRING;
                return TokNull;
            }
            return TokString;
        }

        if (tokbuf[0] == EOS)
            return TokNull;

        /* See if token is a comment to end of line character.	If so, discard
        the rest of the line and return null for this token request. */

        if (strcmp(tokbuf, "\\") == 0) {
            while (*sp != EOS)
                sp++;
            *cp = sp;
            return TokNull;
        }

        /* See if this token is a comment open delimiter.  If so, set to
        ignore all characters until the matching comment close delimiter. */

        if (strcmp(tokbuf, "(") == 0) {
            while (*sp != EOS) {
                if (*sp == ')')
                    break;
                sp++;
            }
            if (*sp == ')') {
                sp++;
                continue;
            }
            atl_comment = Truth;
            *cp = sp;
            return TokNull;
        }

        /* See if the token is a number. */

        if (isdigit(tokbuf[0]) || tokbuf[0] == '-') {
//            char tc;
            char *tcp;

#ifdef USE_SSCANF
            if (sscanf(tokbuf, "%li%c", &tokint, &tc) == 1)
                return TokInt;
#else
            tokint = strtoul(tokbuf, &tcp, 0);
            if (*tcp == 0) {
                return TokInt;
            }
#endif
#ifdef REAL
            if (sscanf(tokbuf, "%lf%c", &tokreal, &tc) == 1)
                return TokReal;
#endif
        }
        return TokWord;
    }
}

/*  LOOKUP  --	Look up token in the dictionary.  */

static dictword *lookup( char *tkname)
{
    dictword *dw = dict;

    ucase(tkname);		      /* Force name to upper case */
    while (dw != NULL) {
        if (!(dw->wname[0] & WORDHIDDEN) && (strcmp(dw->wname + 1, tkname) == 0)) {
#ifdef WORDSUSED
            *(dw->wname) |= WORDUSED; /* Mark this word used */
#endif
            break;
        }
        dw = dw->wnext;
    }
    return dw;
}

#ifdef LINUX

// Stack : addr len fd -- actual
//
prim P_fdRead() {
    char *data = (char *)S2;
    int len=(int)S1;
    int fd = (int)(S0);
    Pop2;

    bzero(data,len);

    ssize_t actual = read(fd,data,len);

    S0=actual;

}

// Stack : addr len fd -- actual
//
prim P_fdWrite() {
    char *data = (char *)S2;
    int len=(int)S1;
    int fd = (int)(S0);
    Pop2;

    ssize_t actual = write(fd,data,len);

    S0=actual;
}

prim P_strstr() {
    char *needle=(char *)S0;
    char *haystack=(char *)S1;
    Pop;

    char *res = strstr(haystack, needle);
    S0=res;
}

prim P_strcasestr() {
    char *needle=(char *)S0;
    char *haystack=(char *)S1;
    Pop;

    char *res = strcasestr(haystack, needle);
    S0=res;
}

#endif

#ifdef LIBSER
prim ATH_wouldBlock() {
    int ret;
    Sl(1);
    So(1);

    int fd = S0;

    bool flag = wouldIblock(fd,0);

    ret = (flag) ? -1 : 0;

    S0 = ret;
}


prim ATH_openSerialPort() {
    Sl(2);
    So(1);

    speed_t baud = (speed_t)S0;
    char *name = (char *)S1;

    int fd = openSerialPort(name, baud);

    Pop;
    S0=fd;

}

prim ATH_closeSerialPort() {
    Sl(1);
    So(0);

    int fd = (int)S0;
    Pop;

    closeSerialPort(fd);
}

prim ATH_flushSerialPort() {
    Sl(1);
    So(0);

    int fd=(int)S0;
    Pop;

    flushSerialPort(fd );
}


#endif
/* Gag me with a spoon!  Does no compiler but Turbo support
#if defined(x) || defined(y) ?? */
#ifdef EXPORT
// #define FgetspNeeded
#endif
#ifdef FILEIO
    #ifndef FgetspNeeded
        #define FgetspNeeded
    #endif
#endif

#ifdef FgetspNeeded
#ifdef FILEIO
/*  ATL_FGETSP	--  Portable database version of FGETS.  This reads the
    next line into a buffer a la fgets().  A line is
    delimited by either a carriage return or a line
    feed, optionally followed by the other character
    of the pair.  The string is always null
    terminated, and limited to the length specified - 1
    (excess characters on the line are discarded.
    The string is returned, or NULL if end of file is
    encountered and no characters were stored.	No end
    of line character is stored in the string buffer.
    */
Exported char *atl_fgetsp(char *s, int n, int stream) {

        int rc=0;
        int ch=0;
        int idx=0;

        while(True) {
//		rc = yaffs_read(stream, &ch, 1);
                rc = read(stream, &ch, 1);
                if(rc < 0) {
                        return errno;
                }
                if(rc == 0) {
                        if(idx == 0) {
                                return NULL;
                        }
                        break;
                }
                if( ch == '\r') {
                        break;
                }
                if( ch == '\n') {
                        break;
                }
                if(idx < (n-1)) {
                        s[idx++] = (ch & 0xff);
                }
        }
        s[idx]='\0';
        return s;
}
#endif
#ifdef LINUX
Exported char *atl_fgetsp(char *s, int n, FILE *stream) {
    int i = 0, ch;

    while (True) {
        ch = getc(stream);
        if (ch == EOF) {
            if (i == 0)
                return NULL;
            break;
        }
        if (ch == '\r') {
            ch = getc(stream);
            if (ch != '\n')
                V ungetc(ch, stream);
            break;
        }
        if (ch == '\n') {
            ch = getc(stream);
            if (ch != '\r')
                V ungetc(ch, stream);
            break;
        }
        if (i < (n - 1))
            s[i++] = ch;
    }
    s[i] = EOS;
    return s;
}
#endif // LINUX
#endif /* FgetspNeeded */

/*  ATL_MEMSTAT  --  Print memory usage summary.  */

#ifdef MEMSTAT
// TODO Comments reduce size to below 255 bytes, now make it print into
// outBuffer
// #warning MEMSTAT
void atl_memstat() {
//    static char fmt[] = "   %-12s %6ld    %6ld    %6ld       %3d %%\r\n";
//     printf("  Memory Area     usage     used    allocated   in use\r\n");

    /*
    printf( fmt, "Stack",
            ((long) (stk - stack)),
            ((long) (stackmax - stack)),
            atl_stklen,
            (100L * (stk - stack)) / atl_stklen);

        printf(fmt, "Return stack",
            ((long) (rstk - rstack)),
            ((long) (rstackmax - rstack)),
            atl_rstklen,
            (100L * (rstk - rstack)) / atl_rstklen);
        printf(fmt, "Heap",
            ((long) (hptr - heap)),
            ((long) (heapmax - heap)),
            atl_heaplen,
            (100L * (hptr - heap)) / atl_heaplen);
#else
*/
    static char fmt[] = "\t%-13s|\t%3ld   |\t%6ld|\t%13ld|\t%3ld %%|\n";
//    static char fmt[] = "\t%-12s %6ld    %6ld    %6ld       %3ld %%\r\n";

    char buffer[80];
    sprintf(buffer,"\n\t+============+========+=======+==============+=======+\n");
    atlastTxString(buffer);

    sprintf(buffer,"\t|Memory Area |  usage |  used |   allocated  |in use |\n");
    atlastTxString(buffer);

    sprintf(buffer,"\t+============+========+=======+==============+=======+\n");
    atlastTxString(buffer);

    int stack_pc = ( 100 * ((stk - stack)) ) / atl_stklen;
    int stack_used = stk - stack ;
    int stack_usage = (stackmax - stack);

    sprintf(buffer,fmt, "|Stack",stack_used, stack_usage,atl_stklen, stack_pc);
    atlastTxString(buffer);

    V sprintf(buffer,fmt, "|Return stack",
            ((long) (rstk - rstack)),
            ((long) (rstackmax - rstack)),
            atl_rstklen,
            (100L * (rstk - rstack)) / atl_rstklen);
    atlastTxString(buffer);

    V sprintf(buffer,fmt, "|Heap        ",
            ((long) (hptr - heap)),
            ((long) (heapmax - heap)),
            atl_heaplen,
            (100L * (hptr - heap)) / atl_heaplen);
    atlastTxString(buffer);

    sprintf(buffer,"\t+============+========+=======+==============+=======+\n");
    atlastTxString(buffer);

}
#endif /* MEMSTAT */

/*  Primitive implementing functions.  */

/*  ENTER  --  Enter word in dictionary.  Given token for word's
    name and initial values for its attributes, returns
    the newly-allocated dictionary item. */

static void enter( char *tkname) {
    /* Allocate name buffer */
    createword->wname = alloc(((unsigned int) strlen(tkname) + 2));
    createword->wname[0] = 0;	      /* Clear flags */
    V strcpy(createword->wname + 1, tkname); /* Copy token to name buffer */
    createword->wnext = dict;	      /* Chain rest of dictionary to word */
    dict = createword;		      /* Put word at head of dictionary */
}

#ifdef Keyhit

/*  KBQUIT  --	If this system allows detecting key presses, handle
    the pause, resume, and quit protocol for the word
    listing facilities.  */

// TODO, not sure what to do here
static Boolean kbquit() {

        Boolean rc=False;
/*
        if(rxReady(console)) {
                (void) rxByte(console );
                rc=True;
        }
        */

        return rc;
        /*
    int key;

    if ((key = Keyhit()) != 0) {
        V printf("\nPress RETURN to stop, any other key to continue: ");
        while ((key = Keyhit()) == 0) ;
        if (key == '\r' || (key == '\n'))
            return True;
    }
    return False;
    */
}
#endif /* Keyhit */

/*  Primitive word definitions.  */

#ifdef NOMEMCHECK
#define Compiling
#else
#define Compiling if (state == Falsity) {notcomp(); return;}
#endif
#define Compconst(x) Ho(1); Hstore = (stackitem) (x)
#define Skipstring ip += *((char *) ip)

prim P_plus()			      /* Add two numbers */
{
    Sl(2);
    /* printf("PLUS %lx + %lx = %lx\n", S1, S0, (S1 + S0)); */
    S1 += S0;
    Pop;
}

prim P_minus()			      /* Subtract two numbers */
{
    Sl(2);
    S1 -= S0;
    Pop;
}

prim P_times()			      /* Multiply two numbers */
{
    Sl(2);
    S1 *= S0;
    Pop;
}

prim P_div()			      /* Divide two numbers */
{
    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    S1 /= S0;
    Pop;
}

prim P_mod()			      /* Take remainder */
{
    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    S1 %= S0;
    Pop;
}

prim P_divmod() 		      /* Compute quotient and remainder */
{
    stackitem quot;

    Sl(2);
#ifndef NOMEMCHECK
    if (S0 == 0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    quot = S1 / S0;
    S1 %= S0;
    S0 = quot;
}

prim P_min()			      /* Take minimum of stack top */
{
    Sl(2);
    S1 = min(S1, S0);
    Pop;
}

prim P_max()			      /* Take maximum of stack top */
{
    Sl(2);
    S1 = max(S1, S0);
    Pop;
}

prim P_neg()			      /* Negate top of stack */
{
    Sl(1);
    S0 = - S0;
}

prim P_abs()			      /* Take absolute value of top of stack */
{
    Sl(1);
    S0 = abs(S0);
}

prim P_equal()			      /* Test equality */
{
    Sl(2);
    S1 = (S1 == S0) ? Truth : Falsity;
    Pop;
}

prim P_unequal()		      /* Test inequality */
{
    Sl(2);
    S1 = (S1 != S0) ? Truth : Falsity;
    Pop;
}

prim P_gtr()			      /* Test greater than */
{
    Sl(2);
    S1 = (S1 > S0) ? Truth : Falsity;
    Pop;
}

prim P_lss()			      /* Test less than */
{
    Sl(2);
    S1 = (S1 < S0) ? Truth : Falsity;
    Pop;
}

prim P_geq()			      /* Test greater than or equal */
{
    Sl(2);
    S1 = (S1 >= S0) ? Truth : Falsity;
    Pop;
}

prim P_leq()			      /* Test less than or equal */
{
    Sl(2);
    S1 = (S1 <= S0) ? Truth : Falsity;
    Pop;
}

// Stack: min max value -- ( min <= value < max )
//
prim P_between() {
    Sl(3);
    So(1);

    int res;


    int value = (int) S0;
    int upperLimit = (int) S1;
    int lowerLimit = (int) S2;
    Pop2;

    res = ( (value >= lowerLimit) && (value < upperLimit)) ? -1 : 0 ;
    S0=res;

}
//
// Stack: addr -- addrn
//
prim P_lstrip() {
    char *ptr=(char *)S0;

    if( *ptr != '\0') {
        while( (*ptr >= 0) && (*ptr < '!')) {
            ptr++;
        }
    }

    S0=(stackitem)ptr;
}


prim P_and()			      /* Logical and */
{
    Sl(2);
    /* printf("AND %lx & %lx = %lx\n", S1, S0, (S1 & S0)); */
    S1 &= S0;
    Pop;
}

prim P_or()			      /* Logical or */
{
    Sl(2);
    S1 |= S0;
    Pop;
}

prim P_xor()			      /* Logical xor */
{
    Sl(2);
    S1 ^= S0;
    Pop;
}

prim P_not()			      /* Logical negation */
{
    Sl(1);
    S0 = ~S0;
}

prim P_shift()			      /* Shift:  value nbits -- value */
{
    Sl(1);
    S1 = (S0 < 0) ? (((unsigned long) S1) >> (-S0)) :
        (((unsigned long) S1) <<   S0);
    Pop;
}

#ifdef SHORTCUTA

prim P_1plus()			      /* Add one */
{
    Sl(1);
    S0++;
}

prim P_2plus()			      /* Add two */
{
    Sl(1);
    S0 += 2;
}

prim P_1minus() 		      /* Subtract one */
{
    Sl(1);
    S0--;
}

prim P_2minus() 		      /* Subtract two */
{
    Sl(1);
    S0 -= 2;
}

prim P_2times() 		      /* Multiply by two */
{
    Sl(1);
    S0 *= 2;
}

prim P_2div()			      /* Divide by two */
{
    Sl(1);
    S0 /= 2;
}

#endif /* SHORTCUTA */

#ifdef SHORTCUTC

prim P_0equal() 		      /* Equal to zero ? */
{
    Sl(1);
    S0 = (S0 == 0) ? Truth : Falsity;
}

prim P_0notequal()		      /* Not equal to zero ? */
{
    Sl(1);
    S0 = (S0 != 0) ? Truth : Falsity;
}

prim P_0gtr()			      /* Greater than zero ? */
{
    Sl(1);
    S0 = (S0 > 0) ? Truth : Falsity;
}

prim P_0lss()			      /* Less than zero ? */
{
    Sl(1);
    S0 = (S0 < 0) ? Truth : Falsity;
}

#endif /* SHORTCUTC */

/*  Storage allocation (heap) primitives  */

prim P_here()			      /* Push current heap address */
{
    So(1);
    Push = (stackitem) hptr;
}

prim P_bang()			      /* Store value into address */
{
    Sl(2);
    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((stackitem *) S0) = S1;
    Pop2;
}

prim P_at()			      /* Fetch value from address */
{
    Sl(1);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 = *((stackitem *) S0);
}

prim P_plusbang()		      /* Add value at specified address */
{
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((stackitem *) S0) += S1;
    Pop2;
}

prim P_allot()			      /* Allocate heap bytes */
{
    stackitem n;

    Sl(1);
    n = (S0 + (sizeof(stackitem) - 1)) / sizeof(stackitem);
    Pop;
    Ho(n);
    hptr += n;
}

prim P_comma()			      /* Store one item on heap */
{
    Sl(1);

//    if ( ath_safe_memory == Truth ) {
//        Hpc(S0);
//    }
    Hstore = S0;
    /*
    t1=S0;
    *hptr = t1;
    hptr++;
    */
    Pop;
}

prim P_cbang()			      /* Store byte value into address */
{
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    *((unsigned char *) S0) = S1;
    Pop2;
}
#ifdef ATH

prim ATH_mkBuffer() {
    /* Declare constant */
    void *hp = hptr;
    int size;
    void *t;

    Sl(1);

    size=S0;
    Pop;

    P_create(); 		      /* Create dictionary item */

    createword->wcode = P_con;	      /* Set code to constant push */

    Ho(1)

    size = ((size + (sizeof(stackitem) - 1 )) / sizeof(stackitem));  // no of items in units of stacksize
    size = size * sizeof(stackitem);        // Size in bytes

    t = hp+(4*sizeof(stackitem));		      /* Store constant value in body */

    Hstore = (stackitem)t;
    hptr+=size;

    memset(t,0,size);
}

prim ATH_wbang() {
    Sl(2);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }

    uint16_t *ptr=(uint16_t *)S0;
    uint16_t data=S1;

    *ptr=(uint16_t)(data & 0xffff);

    Pop2;
}

/* Fetch byte value from address */
prim ATH_wat() {

    Sl(1);
    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 = *((uint16_t *) S0);
}

prim ATH_16toCell() {
        stackitem v;

        v=(stackitem)(int16_t)S0 ;
        S0=v;


}
#endif

prim P_cat()			      /* Fetch byte value from address */
{
    Sl(1);

    if ( ath_safe_memory == Truth ) {
        Hpc(S0);
    }
    S0 = *((unsigned char *) S0);
}

prim P_ccomma() 		      /* Store one byte on heap */
{
    unsigned char *chp;
    void *tmp;

    Sl(1);
    tmp = (void *)S0;

    if ( ath_safe_memory == Truth ) {
        Hpc(tmp);
//        Hpc(S0);
    }
    chp = ((unsigned char *) hptr);
    *chp++ = S0;
    hptr = (stackitem *) chp;
    Pop;
}

prim P_cequal() 		      /* Align heap pointer after storing */
{				      /* a series of bytes. */
    stackitem n = (((stackitem) hptr) - ((stackitem) heap)) %
        (sizeof(stackitem));

    if (n != 0) {
        char *chp = ((char *) hptr);

        chp += sizeof(stackitem) - n;
        hptr = ((stackitem *) chp);
    }
}

/*  Variable and constant primitives  */

prim P_var()			      /* Push body address of current word */
{
    So(1);
    Push = (stackitem) (((stackitem *) curword) + Dictwordl);
}

prim P_create()	      /* Create new word */
{
    defpend = True;		      /* Set definition pending */
    Ho(Dictwordl);
    createword = (dictword *) hptr;   /* Develop address of word */
    createword->wname = NULL;	      /* Clear pointer to name string */
    createword->wcode = P_var;	      /* Store default code */
    hptr += Dictwordl;		      /* Allocate heap space for word */
}

prim P_forget() 		      /* Forget word */
{
    forgetpend = True;		      /* Mark forget pending */
}

prim P_variable()		      /* Declare variable */
{
    P_create(); 		      /* Create dictionary item */
    Ho(1);
    Hstore = 0; 		      /* Initial value = 0 */
}

prim P_con()			      /* Push value in body */
{
    So(1);
    Push = *(((stackitem *) curword) + Dictwordl);
}

prim P_constant()		      /* Declare constant */
{
    Sl(1);
    P_create(); 		      /* Create dictionary item */
    createword->wcode = P_con;	      /* Set code to constant push */
    Ho(1);
    Hstore = S0;		      /* Store constant value in body */
    Pop;
}

/*  Array primitives  */

#ifdef ARRAY
prim P_arraysub() /* Array subscript calculation */
{            /* sub1 sub2 ... subn -- addr */
    int i, offset, esize, nsubs;
    stackitem *array;
    stackitem *isp;

    Sl(1);
    array = (((stackitem *) curword) + Dictwordl);
    Hpc(array);
    nsubs = *array++;		      /* Load number of subscripts */
    esize = *array++;		      /* Load element size */
#ifndef NOMEMCHECK
    isp = &S0;
    for (i = 0; i < nsubs; i++) {
        stackitem subn = *isp--;

        if (subn < 0 || subn >= array[i])
            trouble("Subscript out of range");
    }
#endif /* NOMEMCHECK */
    isp = &S0;
    offset = *isp;		      /* Load initial offset */
    for (i = 1; i < nsubs; i++)
        offset = (offset * (*(++array))) + *(--isp);
    Npop(nsubs - 1);
    /* Calculate subscripted address.  We start at the current word,
    advance to the body, skip two more words for the subscript count
    and the fundamental element size, then skip the subscript bounds
    words (as many as there are subscripts).  Then, finally, we
    can add the calculated offset into the array. */
    S0 = (stackitem) (((char *) (((stackitem *) curword) +
                    Dictwordl + 2 + nsubs)) + (esize * offset));
}

prim P_array()			      /* Declare array */
{				      /* sub1 sub2 ... subn n esize -- array */
    int i, nsubs, asize = 1;
    stackitem *isp;

    Sl(2);
#ifndef NOMEMCHECK
    if (S0 <= 0)
        trouble("Bad array element size");
    if (S1 <= 0)
        trouble("Bad array subscript count");
#endif /* NOMEMCHECK */

    nsubs = S1; 		      /* Number of subscripts */
    Sl(nsubs + 2);		      /* Verify that dimensions are present */

    /* Calculate size of array as the product of the subscripts */

    asize = S0; 		      /* Fundamental element size */
    isp = &S2;
    for (i = 0; i < nsubs; i++) {
#ifndef NOMEMCHECK
        if (*isp <= 0)
            trouble("Bad array dimension");
#endif /* NOMEMCHECK */
        asize *= *isp--;
    }

    asize = (asize + (sizeof(stackitem) - 1)) / sizeof(stackitem);
    Ho(asize + nsubs + 2);	      /* Reserve space for array and header */
    P_create(); 		      /* Create variable */
    createword->wcode = P_arraysub;   /* Set method to subscript calculate */
    Hstore = nsubs;		      /* Header <- Number of subscripts */
    Hstore = S0;		      /* Header <- Fundamental element size */
    isp = &S2;
    for (i = 0; i < nsubs; i++) {     /* Header <- Store subscripts */
        Hstore = *isp--;
    }
    while (asize-- > 0) 	      /* Clear the array to zero */
        Hstore = 0;
    Npop(nsubs + 2);
}
#endif /* ARRAY */

/*  String primitives  */

#ifdef STRING

prim P_strlit() 		      /* Push address of string literal */
{
    So(1);
    Push = (stackitem) (((char *) ip) + 1);
#ifdef TRACE
    if (atl_trace) {
#ifdef MBED
        char buffer[80];
        sprintf(buffer,"\"%s\" ", (((char *) ip) + 1)); // EMBEDDED
        atlastTxString(buffer);
#endif
    }
#endif /* TRACE */
    Skipstring; 		      /* Advance IP past it */
}

prim P_string() 		      /* Create string buffer */
{
    Sl(1);
    Ho((S0 + 1 + sizeof(stackitem)) / sizeof(stackitem));
    P_create(); 		      /* Create variable */
    /* Allocate storage for string */
    hptr += (S0 + 1 + sizeof(stackitem)) / sizeof(stackitem);
    Pop;
}

// ATH, raw memory move.
//
prim P_move() {
    Sl(3);

    uint32_t len = S0 ;
    char *dest=(char *)S1;
    char *src =(char *)S2;

    memcpy( dest, src, len);
}
//
// ATH find token
//
// Stack: addr char - addr
prim P_strsep() {
    uint8_t *buffer;
    uint8_t *res=NULL;
    uint8_t delim;

    Sl(2);
    So(1);

    delim=(uint8_t) S0;
    buffer=(uint8_t *)S1;

    res=strsep((char **)&buffer, (char *)&delim);
    Pop2;
    Push=(stackitem)res;
}

prim P_strcpy() 		      /* Copy string to address on stack */
{
    Sl(2);
    // Hpc checks that the pointer is an address inside the heap.
    // This prevents you writing to random memory addresses.
    // However sometimes you want to write to memory that has been
    // allocated by malloc, or a hardware address.
    // NOTE Use with caution
    // TODO Should I set ath_safe_memory to Truth after use ?  this means
    // every acces to memory outside of heap would have to be prefixed with
    // 0 memsafe
    //
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    V strcpy((char *) S0, (char *) S1);
    Pop2;
}

prim P_strcat() 		      /* Append string to address on stack */
{
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    V strcat((char *) S0, (char *) S1);
    Pop2;
}

prim P_strlen() 		      /* Take length of string on stack top */
{
    Sl(1);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
    }
    S0 = strlen((char *) S0);
}

prim P_strcmp() 		      /* Compare top two strings on stack */
{
    int i;

    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    i = strcmp((char *) S1, (char *) S0);
    S1 = (i == 0) ? 0L : ((i > 0) ? 1L : -1L);
    Pop;
}

prim P_strchar()		      /* Find character in string */
{
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }
    S1 = (stackitem) strchr((char *) S1, *((char *) S0));
    Pop;
}

prim P_substr() 		      /* Extract and store substring */
{				      /* source start length/-1 dest -- */
    long sl, sn;
    char *ss, *sp, *se, *ds;

    Sl(4);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S3);
    }
    sl = strlen(ss = ((char *) S3));
    se = ss + sl;
    sp = ((char *) S3) + S2;
    if ((sn = S1) < 0)
        sn = 999999L;
    ds = (char *) S0;
    while (sn-- && (sp < se))
        *ds++ = *sp++;
    *ds++ = EOS;
    Npop(4);
}

prim P_strform()		      /* Format integer using sprintf() */
{                                     /* value "%ld" str -- */
    Sl(2);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
        Hpc(S1);
    }

    V sprintf((char *) S0, (char *) S1, S2);  // NOT EMBEDDED

    Npop(3);
}

#ifdef REAL
prim P_fstrform()		      /* Format real using sprintf() */
{                                     /* rvalue "%6.2f" str -- */
    Sl(4);
    Hpc(S0);
    Hpc(S1);
    V sprintf((char *) S0, (char *) S1, REAL1);
    Npop(4);
}
#endif /* REAL */

prim P_strint() 		      /* String to integer */
{				      /* str -- endptr value */
    stackitem is;
    char *eptr;

    Sl(1);
    So(1);
    if( ath_safe_memory == Truth) {
        Hpc(S0);
    }
    is = strtoul((char *) S0, &eptr, 0);
    S0 = (stackitem) eptr;
    Push = is;
}

#ifdef REAL
prim P_strreal()		      /* String to real */
{				      /* str -- endptr value */
    int i;
    union {
        atl_real fs;
        stackitem fss[Realsize];
    } fsu;
    char *eptr;

    Sl(1);
    So(2);
    Hpc(S0);
    fsu.fs = strtod((char *) S0, &eptr);
    S0 = (stackitem) eptr;
    for (i = 0; i < Realsize; i++) {
        Push = fsu.fss[i];
    }
}
#endif /* REAL */
#endif /* STRING */

/*  Floating point primitives  */

#ifdef REAL

prim P_flit()			      /* Push floating point literal */
{
    int i;

    So(Realsize);
#ifdef TRACE
    if (atl_trace) {
        atl_real tr;

        V memcpy((char *) &tr, (char *) ip, sizeof(atl_real));
#ifdef MBED
        char buffer[80];

        sprintf(buffer,"%g ", tr);
        atlastTxString(buffer);
#endif
    }
#endif /* TRACE */
    for (i = 0; i < Realsize; i++) {
        Push = (stackitem) *ip++;
    }
}

prim P_fplus()			      /* Add floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 + REAL0);
    Realpop;
}

prim P_fminus() 		      /* Subtract floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 - REAL0);
    Realpop;
}

prim P_ftimes() 		      /* Multiply floating point numbers */
{
    Sl(2 * Realsize);
    SREAL1(REAL1 * REAL0);
    Realpop;
}

prim P_fdiv()			      /* Divide floating point numbers */
{
    Sl(2 * Realsize);
#ifndef NOMEMCHECK
    if (REAL0 == 0.0) {
        divzero();
        return;
    }
#endif /* NOMEMCHECK */
    SREAL1(REAL1 / REAL0);
    Realpop;
}

prim P_fmin()			      /* Minimum of top two floats */
{
    Sl(2 * Realsize);
    SREAL1(min(REAL1, REAL0));
    Realpop;
}

prim P_fmax()			      /* Maximum of top two floats */
{
    Sl(2 * Realsize);
    SREAL1(max(REAL1, REAL0));
    Realpop;
}

prim P_fneg()			      /* Negate top of stack */
{
    Sl(Realsize);
    SREAL0(- REAL0);
}

prim P_fabs()			      /* Absolute value of top of stack */
{
    Sl(Realsize);
    SREAL0(abs(REAL0));
}

prim P_fequal() 		      /* Test equality of top of stack */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 == REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_funequal()		      /* Test inequality of top of stack */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 != REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fgtr()			      /* Test greater than */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 > REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_flss()			      /* Test less than */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 < REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fgeq()			      /* Test greater than or equal */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 >= REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fleq()			      /* Test less than or equal */
{
    stackitem t;

    Sl(2 * Realsize);
    t = (REAL1 <= REAL0) ? Truth : Falsity;
    Realpop2;
    Push = t;
}

prim P_fdot()			      /* Print floating point top of stack */
{
    Sl(Realsize);

#ifdef MBED
    char buffer[80];
    sprintf(buffer,"%g ", REAL0);
#endif

    Realpop;
}

prim P_float()			      /* Convert integer to floating */
{
    atl_real r;

    Sl(1)
        So(Realsize - 1);
    r = S0;
    stk += Realsize - 1;
    SREAL0(r);
}

prim P_fix()			      /* Convert floating to integer */
{
    stackitem i;

    Sl(Realsize);
    i = (int) REAL0;
    Realpop;
    Push = i;
}

#ifdef MATH

#define Mathfunc(x) Sl(Realsize); SREAL0(x(REAL0))

prim P_acos()			      /* Arc cosine */
{
    Mathfunc(acos);
}

prim P_asin()			      /* Arc sine */
{
    Mathfunc(asin);
}

prim P_atan()			      /* Arc tangent */
{
    Mathfunc(atan);
}

prim P_atan2()			      /* Arc tangent:  y x -- atan */
{
    Sl(2 * Realsize);
    SREAL1(atan2(REAL1, REAL0));
    Realpop;
}

prim P_cos()			      /* Cosine */
{
    Mathfunc(cos);
}

prim P_exp()			      /* E ^ x */
{
    Mathfunc(exp);
}

prim P_log()			      /* Natural log */
{
    Mathfunc(log);
}

prim P_pow()			      /* X ^ Y */
{
    Sl(2 * Realsize);
    SREAL1(pow(REAL1, REAL0));
    Realpop;
}

prim P_sin()			      /* Sine */
{
    Mathfunc(sin);
}

prim P_sqrt()			      /* Square root */
{
    Mathfunc(sqrt);
}

prim P_tan()			      /* Tangent */
{
    Mathfunc(tan);
}
#undef Mathfunc
#endif /* MATH */
#endif /* REAL */

/*  Console I/O primitives  */

#ifdef CONIO

/* Print top of stack, pop it */
prim P_dot() {
    Sl(1);
//    stackitem top=S0;
    int top=S0;
//    char outBuffer[32];
    char buff[32];

    switch(base) {
    case 10:
        sprintf(buff,"%d",top);
        break;
    case 16:
        sprintf(buff,"%x",top);
        break;
    default:
        sprintf(buff,"%d",top);
        break;
    }
#ifdef MBED
    atlastTxString(buff);
#endif

    Pop;
}

prim P_question()		      /* Print value at address */
{
    Sl(1);
    Hpc(S0);
#ifdef MBED
    char outBuffer[16];
    sprintf(outBuffer,(base == 16 ? "%lX" : "%ld "), *((stackitem *) S0)); // EMBEDDED
    atlastTxString(outBuffer);
#endif
/*
#ifdef EMBEDDED
//    sprintf(outBuffer,(base == 16 ? "%lX" : "%ld "), *((stackitem *) S0)); // EMBEDDED
    sprintf(outBuffer,"Hello\n");
#endif
    printf("%s",outBuffer);
#endif
*/
    Pop;
}

/* Carriage return */
prim P_cr() {
#ifdef MBED
    atlastTxByte('\n');
#endif
}

/* Print entire contents of stack */
prim P_dots() {
        stackitem *tsp;
    char outBuffer[16];

        sprintf(outBuffer,"\nStack: ");    // NOT EMBEDDED
#ifdef MBED
    atlastTxString(outBuffer);
#endif

        if (stk == stackbot) {

                sprintf(outBuffer,"Empty.");  // EMBEDDED
#ifdef MBED
        atlastTxString(outBuffer);
#endif

        } else {
                for (tsp = stack; tsp < stk; tsp++) {
                        // TODO If you change the stack size change this
                        sprintf(outBuffer,(base == 16 ? "%lX " : "%ld "), *tsp); //  EMBEDDED
#ifdef MBED
        atlastTxString(outBuffer);
#endif
                }
        }
}

prim P_dotquote()		      /* Print literal string that follows */
{
    Compiling;
    stringlit = True;		      /* Set string literal expected */
    Compconst(s_dotparen);	      /* Compile .( word */
}

/* Print literal string that follows */
prim P_dotparen() {
    char buffer[80];
    if (ip == NULL) {		      /* If interpreting */
        stringlit = True;	      /* Set to print next string constant */
    } else {			      /* Otherwise, */
        /* print string literal in in-line code. */
        sprintf(buffer,"%s", ((char *) ip) + 1);
#ifdef MBED
        atlastTxString(buffer);
#endif
        Skipstring;		      /* And advance IP past it */
    }
}


// send the byte at TOS.
prim P_emit() {
        Sl(1);

#ifdef FREERTOS
        atlastTxByte(S0);
#endif
#ifdef LINUX
    putchar(S0);
#endif
        Pop;
}
/* Print string pointed to by stack */

prim P_type() {
    char buffer[80];
    Sl(1);

    if( ath_safe_memory == Truth) {
        Hpc(S0);
    }

    sprintf(buffer,"%s", (char *) S0);

#ifdef MBED
    atlastTxString(buffer);
#endif

    Pop;
}

prim ATH_sift() {
    char outBuffer[132];

    char *res=NULL;

    Sl(1);
    So(0);

    char *name = (char *)S0;

    Pop;
    dictword *dw = dict;
    while (dw != NULL) {
        res=strcasestr( dw->wname+1, name);
        if( res != NULL) {
            sprintf(outBuffer,"%s\n", dw->wname+1);
#ifdef MBED
            atlastTxString(outBuffer);
#endif
        }
        dw = dw->wnext;
    }

}

/* List words */
prim P_words() {
    char outBuffer[132];
// extern char outBuffer[];
#ifndef Keyhit
    int key = 0;
#endif
    dictword *dw = dict;

    while (dw != NULL) {
        strcpy(outBuffer,"\r\n");
        strcat(outBuffer,dw->wname+1);
//    	strcat(outBuffer,"\r\n");

    atlastTxString(outBuffer);

        dw = dw->wnext;
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#else
        /* If this system can't trap keystrokes, just stop the WORDS
    NOT       listing after 20 words. */
        if (++key >= 200)
            break;
#endif
    }
    sprintf(outBuffer,"\n"); // EMBEDDED
    atlastTxString(outBuffer);
}
#endif /* CONIO */

#ifdef FILEIO

prim P_file()			      /* Declare file */
{
    Ho(2);
    P_create(); 		      /* Create variable */
    Hstore = FileSent;		      /* Store file sentinel */
    Hstore = 0; 		      /* Mark file not open */
}

prim P_mkdir() {
    mode_t mode=0666; // By default make it globally accesible.
}

prim P_rmdir() {
}

/* Open file: fname fmodes fd -- flag */
/*
prim P_fopen()	{
    stackitem stat;

    Sl(3);
    Hpc(S2);
    Hpc(S0);

//    Isfile(S0);

//    FILE *fd = fopen((char *) S2, fopenmodes[S1]);

    char * fname = (char *) S2;
    char *mode =(char *)S1;

    if(!fd) {
        error("error: %s (%d)\r\n", strerror(errno), -errno);
    }

//    FILE *fd = fopen(fname, mode);

    if (fd == NULL) {
        stat = Falsity;
    } else {
        *(((stackitem *) S0) + 1) = (stackitem) fd;
        stat = Truth;
    }

    Pop2;
    S0 = stat;
}
*/

#if 0
prim P_fclose() 		      /* Close file: fd -- */
{
    Sl(1);
    Hpc(S0);

    Isfile(S0);
    Isopen(S0);
    V fclose(FileD(S0));

    *(((stackitem *) S0) + 1) = (stackitem) NULL;
    Pop;
}
#endif

prim P_fdelete()		      /* Delete file: fname -- flag */
{
    Sl(1);
    Hpc(S0);
#ifdef LINUX
    S0 = (unlink((char *) S0) == 0) ? Truth : Falsity;
#endif
#if defined(FREERTOS) && defined(YAFFS)
    S0 = (yaffs_unlink((char *) S0) == 0) ? Truth : Falsity;
#endif
}

prim P_fgetline()		      /* Get line: fd string -- flag */
{
    Sl(2);
    Hpc(S0);
    Isfile(S1);
    Isopen(S1);
    if (atl_fgetsp((char *) S0, 132, FileD(S1)) == NULL) {
        S1 = Falsity;
    } else {
        S1 = Truth;
    }
    Pop;
}

prim P_fputline()		      /* Put line: string fd -- flag */
{
    Sl(2);
    Hpc(S1);
    Isfile(S0);
    Isopen(S0);
    if (fputs((char *) S1, FileD(S0)) == EOF) {
        S1 = Falsity;
    } else {
        S1 = putc('\n', FileD(S0)) == EOF ? Falsity : Truth;
    }
    Pop;
}

#if 0
prim P_fread()			      {
    /* Was ------- File read: fd len buf -- length */
    /* ATH Now --- File read: buf len fd -- length */
    Sl(3);
    Hpc(S0);

    Isfile(S0);
    Isopen(S0);
    // TODO This is stupid, it is inconsisitent with write.
    // The stack order should follow the C convention.
    // S2 = fread((char *) S0, 1, ((int) S1), FileD(S2));
    S2 = fread((char *) S2, 1, ((int) S1), FileD(S0));

    Pop2;
}

/* File write: len buf fd -- length */
prim P_fwrite() {
    Sl(3);
    Hpc(S2);
    Isfile(S0);

    Isopen(S0);
    S2 = fwrite((char *) S1, 1, ((int) S2), FileD(S0));
    Pop2;
}
#endif

prim P_fgetc()			      /* File get character: fd -- char */
{
    Sl(1);
    Isfile(S0);
    Isopen(S0);
    S0 = getc(FileD(S0));	      /* Returns -1 if EOF hit */
}

prim P_fputc()			      /* File put character: char fd -- stat */
{
    Sl(2);
    Isfile(S0);
    Isopen(S0);
    S1 = putc((char) S1, FileD(S0));
    Pop;
}

prim P_ftell()			      /* Return file position:	fd -- pos */
{
    Sl(1);
    Isfile(S0);
    Isopen(S0);
    S0 = (stackitem) ftell(FileD(S0));
}

#if 0
prim P_fseek()			      /* Seek file:  offset base fd -- */
{
    Sl(3);
    So(1);
    Isfile(S0);
    Isopen(S0);

    unsigned long roffset=0;
#ifdef LINUX
    V fseek(FileD(S0), (long) S2, (int) S1);
#endif

#if defined(FREERTOS) && defined(YAFFS)
    roffset = yaffs_lseek(FileD(S0), (long) S2, (int) S1);
#endif
    Npop(2);
    S0 = roffset;
}
#endif

prim P_access() {
        Sl(2);
        So(1);

#if defined(FREERTOS) && defined(YAFFS)

        S1 =  yaffs_access((char *)S1, (int) S0);

        Pop;
#endif

}

#if defined(FREERTOS) && defined(YAFFS)
prim FR_loadFile() {
        Sl(1);
        So(0);

        int fd=-1;
        /*
        char buffer[2048];
        int idx=0;

        memset(buffer,'A', 64);
        fd = yaffs_open("NAND/crap.txt", O_WRONLY|O_CREAT,0666);

        for(idx=0;idx<10;idx++) {
                yaffs_write(fd, buffer, 64);
        }
        */

        char *fname=S0;
        char buffer[132];
        bool runFlag=True;

        Pop;
        V yaffs_unlink(fname);

        fd = yaffs_open(fname, O_WRONLY|O_CREAT,0666);

        while(runFlag) {
                Push=(stackitem)buffer;
                Push=132;

                FR_consoleExpect();


                if(!strcmp("\\ EOF\n", buffer)) {
                        runFlag=False;
                } else {
                        yaffs_write(fd, buffer, strlen(buffer));
                }
        }

        V yaffs_close(fd);
}
#endif

prim P_fload()			      /* Load source file:  fd -- evalstat */
{
    int estat;
    FILE *fd;

    Sl(1);
    Isfile(S0);
    Isopen(S0);
    fd = FileD(S0);
    Pop;
    estat = atl_load(fd);
    So(1);
    Push = estat;
}

prim P_include() {
    int estat;
    Sl(1);
    FILE *fd;

    fd = fopen((char *)S0, "r") ;

    if(!fd) {
        perror("INCLUDE fopen");
        Pop;
        return;
    }
    estat = atl_load(fd);

    fclose(fd);

    Pop;
}

#endif /* FILEIO */

#ifdef EVALUATE

prim P_evaluate()
{				      /* string -- status */
    int es = ATL_SNORM;
    atl_statemark mk;
    atl_int scomm = atl_comment;      /* Stack comment pending state */
    dictword **sip = ip;	      /* Stack instruction pointer */
    char *sinstr = instream;	      /* Stack input stream */
    char *estring;

    Sl(1);
    Hpc(S0);
    estring = (char *) S0;	      /* Get string to evaluate */
    Pop;			      /* Pop so it sees arguments below it */
    atl_mark(&mk);		      /* Mark in case of error */
    ip = NULL;			      /* Fool atl_eval into interp state */
    if ((es = atl_eval(estring)) != ATL_SNORM) {
        atl_unwind(&mk);
    }
    /* If there were no other errors, check for a runaway comment.  If
    we ended the file in comment-ignore mode, set the runaway comment
    error status and unwind the file.  */
    if ((es == ATL_SNORM) && (atl_comment != 0)) {
        es = ATL_RUNCOMM;
        atl_unwind(&mk);
    }
    atl_comment = scomm;	      /* Unstack comment pending status */
    ip = sip;			      /* Unstack instruction pointer */
    instream = sinstr;		      /* Unstack input stream */
    So(1);
    Push = es;			      /* Return eval status on top of stack */
}
#endif /* EVALUATE */

/*  Stack mechanics  */

int depth() {
    return (stk - stack);
}

/* Push stack depth */
prim P_depth() {

    stackitem s = stk - stack;

    So(1);
    Push = s;
}

prim P_clear()			      /* Clear stack */
{
    stk = stack;
}

prim P_dup()			      /* Duplicate top of stack */
{
    stackitem s;

    Sl(1);
    So(1);
    s = S0;
    Push = s;
}

prim P_drop()			      /* Drop top item on stack */
{
    Sl(1);
    Pop;
}

prim P_swap()			      /* Exchange two top items on stack */
{
    stackitem t;

    Sl(2);
    t = S1;
    S1 = S0;
    S0 = t;
}

prim P_nip() {
        S1=S0;
        Pop;
}

prim P_over()			      /* Push copy of next to top of stack */
{
    stackitem s;

    Sl(2);
    So(1);
    s = S1;
    Push = s;
}

prim P_pick()			      /* Copy indexed item from stack */
{
    Sl(2);
    S0 = stk[-(2 + S0)];
}

prim P_rot()			      /* Rotate 3 top stack items */
{
    stackitem t;

    Sl(3);
    t = S0;
    S0 = S2;
    S2 = S1;
    S1 = t;
}

prim P_minusrot()		      /* Reverse rotate 3 top stack items */
{
    stackitem t;

    Sl(3);
    t = S0;
    S0 = S1;
    S1 = S2;
    S2 = t;
}

prim P_roll()			      /* Rotate N top stack items */
{
    stackitem i, j, t;

    Sl(1);
    i = S0;
    Pop;
    Sl(i + 1);
    t = stk[-(i + 1)];
    for (j = -(i + 1); j < -1; j++)
        stk[j] = stk[j + 1];
    S0 = t;
}

prim P_tor()			      /* Transfer stack top to return stack */
{
    Rso(1);
    Sl(1);
    Rpush = (rstackitem) S0;
    Pop;
}

prim P_rfrom()			      /* Transfer return stack top to stack */
{
    Rsl(1);
    So(1);
    Push = (stackitem) R0;
    Rpop;
}

prim P_rfetch() 		      /* Fetch top item from return stack */
{
    Rsl(1);
    So(1);
    Push = (stackitem) R0;
}

#ifdef Macintosh
/* This file creates more than 32K of object code on the Mac, which causes
MPW to barf.  So, we split it up into two code segments of <32K at this
point. */
#pragma segment TOOLONG
#endif /* Macintosh */

/*  Double stack manipulation items  */

#ifdef DOUBLE

prim P_2dup()			      /* Duplicate stack top doubleword */
{
    stackitem s;

    Sl(2);
    So(2);
    s = S1;
    Push = s;
    s = S1;
    Push = s;
}

prim P_2drop()			      /* Drop top two items from stack */
{
    Sl(2);
    stk -= 2;
}

prim P_2swap()			      /* Swap top two double items on stack */
{
    stackitem t;

    Sl(4);
    t = S2;
    S2 = S0;
    S0 = t;
    t = S3;
    S3 = S1;
    S1 = t;
}

prim P_2over()			      /* Extract second pair from stack */
{
    stackitem s;

    Sl(4);
    So(2);
    s = S3;
    Push = s;
    s = S3;
    Push = s;
}

prim P_2rot()			      /* Move third pair to top of stack */
{
    stackitem t1, t2;

    Sl(6);
    t2 = S5;
    t1 = S4;
    S5 = S3;
    S4 = S2;
    S3 = S1;
    S2 = S0;
    S1 = t2;
    S0 = t1;
}

prim P_2variable()		      /* Declare double variable */
{
    P_create(); 		      /* Create dictionary item */
    Ho(2);
    Hstore = 0; 		      /* Initial value = 0... */
    Hstore = 0; 		      /* ...in both words */
}

prim P_2con()			      /* Push double value in body */
{
    So(2);
    Push = *(((stackitem *) curword) + Dictwordl);
    Push = *(((stackitem *) curword) + Dictwordl + 1);
}

prim P_2constant()		      /* Declare double word constant */
{
    Sl(1);
    P_create(); 		      /* Create dictionary item */
    createword->wcode = P_2con;       /* Set code to constant push */
    Ho(2);
    Hstore = S1;		      /* Store double word constant value */
    Hstore = S0;		      /* in the two words of body */
    Pop2;
}

prim P_2bang()			      /* Store double value into address */
{
    stackitem *sp;

    Sl(2);
    Hpc(S0);
    sp = (stackitem *) S0;
    *sp++ = S2;
    *sp = S1;
    Npop(3);
}

prim P_2at()			      /* Fetch double value from address */
{
    stackitem *sp;

    Sl(1);
    So(1);
    Hpc(S0);
    sp = (stackitem *) S0;
    S0 = *sp++;
    Push = *sp;
}
#endif /* DOUBLE */

/*  Data transfer primitives  */

prim P_dolit()			      /* Push instruction stream literal */
{
    So(1);
#ifdef TRACE
    char outBuffer[16];
    if (atl_trace) {

        sprintf(outBuffer,"%ld ", (long) *ip);
#ifdef MBED
        atlastTxString(outBuffer);
#endif

    }
#endif
    Push = (stackitem) *ip++;	      /* Push the next datum from the
                                        instruction stream. */
}

/*  Control flow primitives  */

prim P_nest()			      /* Invoke compiled word */
{
    Rso(1);
#ifdef WALKBACK
    *wbptr++ = curword; 	      /* Place word on walkback stack */
#endif
    Rpush = ip; 		      /* Push instruction pointer */
    ip = (((dictword **) curword) + Dictwordl);
}

prim P_exit()			      /* Return to top of return stack */
{
    Rsl(1);
#ifdef WALKBACK
    wbptr = (wbptr > wback) ? wbptr - 1 : wback;
#endif
    ip = R0;			      /* Set IP to top of return stack */
    Rpop;
}

prim P_branch() 		      /* Jump to in-line address */
{
    ip += (stackitem) *ip;	      /* Jump addresses are IP-relative */
}

prim P_qbranch()		      /* Conditional branch to in-line addr */
{
    Sl(1);
    if (S0 == 0)		      /* If flag is false */
        ip += (stackitem) *ip;	      /* then branch. */
    else			      /* Otherwise */
        ip++;			      /* skip the in-line address. */
    Pop;
}

prim P_if()			      /* Compile IF word */
{
    Compiling;
    Compconst(s_qbranch);	      /* Compile question branch */
    So(1);
    Push = (stackitem) hptr;	      /* Save backpatch address on stack */
    Compconst(0);		      /* Compile place-holder address cell */
}

prim P_else()			      /* Compile ELSE word */
{
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_branch);	      /* Compile branch around other clause */
    Compconst(0);		      /* Compile place-holder address cell */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get IF backpatch address */
    *bp = hptr - bp;
    S0 = (stackitem) (hptr - 1);      /* Update backpatch for THEN */
}

prim P_then()			      /* Compile THEN word */
{
    stackitem *bp;

    Compiling;
    Sl(1);
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get IF/ELSE backpatch address */
    *bp = hptr - bp;
    Pop;
}

prim P_qdup()			      /* Duplicate if nonzero */
{
    Sl(1);
    if (S0 != 0) {
        stackitem s = S0;
        So(1);
        Push = s;
    }
}

prim P_begin()			      /* Compile BEGIN */
{
    Compiling;
    So(1);
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_until()			      /* Compile UNTIL */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_qbranch);	      /* Compile question branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    Pop;
}

prim P_again()			      /* Compile AGAIN */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Compconst(s_branch);	      /* Compile unconditional branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    Pop;
}

prim P_while()			      /* Compile WHILE */
{
    Compiling;
    So(1);
    Compconst(s_qbranch);	      /* Compile question branch */
    Compconst(0);		      /* Compile place-holder address cell */
    Push = (stackitem) (hptr - 1);    /* Queue backpatch for REPEAT */
}

prim P_repeat() 		      /* Compile REPEAT */
{
    stackitem off;
    stackitem *bp1, *bp;

    Compiling;
    Sl(2);
    Hpc(S0);
    bp1 = (stackitem *) S0;	      /* Get WHILE backpatch address */
    Pop;
    Compconst(s_branch);	      /* Compile unconditional branch */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get BEGIN address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *bp1 = hptr - bp1;                /* Backpatch REPEAT's jump out of loop */
    Pop;
}

prim P_do()			      /* Compile DO */
{
    Compiling;
    Compconst(s_xdo);		      /* Compile runtime DO word */
    So(1);
    Compconst(0);		      /* Reserve cell for LEAVE-taking */
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_xdo()			      /* Execute DO */
{
    Sl(2);
    Rso(3);
    Rpush = ip + ((stackitem) *ip);   /* Push exit address from loop */
    ip++;			      /* Increment past exit address word */
    Rpush = (rstackitem) S1;	      /* Push loop limit on return stack */
    Rpush = (rstackitem) S0;	      /* Iteration variable initial value to
                                        return stack */
    stk -= 2;
}

prim P_qdo()			      /* Compile ?DO */
{
    Compiling;
    Compconst(s_xqdo);		      /* Compile runtime ?DO word */
    So(1);
    Compconst(0);		      /* Reserve cell for LEAVE-taking */
    Push = (stackitem) hptr;	      /* Save jump back address on stack */
}

prim P_xqdo()			      /* Execute ?DO */
{
    Sl(2);
    if (S0 == S1) {
        ip += (stackitem) *ip;
    } else {
        Rso(3);
        Rpush = ip + ((stackitem) *ip);/* Push exit address from loop */
        ip++;			      /* Increment past exit address word */
        Rpush = (rstackitem) S1;      /* Push loop limit on return stack */
        Rpush = (rstackitem) S0;      /* Iteration variable initial value to
                                        return stack */
    }
    stk -= 2;
}

prim P_loop()			      /* Compile LOOP */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_xloop); 	      /* Compile runtime loop */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get DO address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *(bp - 1) = (hptr - bp) + 1;      /* Backpatch exit address offset */
    Pop;
}

prim P_ploop()			      /* Compile +LOOP */
{
    stackitem off;
    stackitem *bp;

    Compiling;
    Sl(1);
    Compconst(s_pxloop);	      /* Compile runtime +loop */
    Hpc(S0);
    bp = (stackitem *) S0;	      /* Get DO address */
    off = -(hptr - bp);
    Compconst(off);		      /* Compile negative jumpback address */
    *(bp - 1) = (hptr - bp) + 1;      /* Backpatch exit address offset */
    Pop;
}

prim P_xloop()			      /* Execute LOOP */
{
    Rsl(3);
    R0 = (rstackitem) (((stackitem) R0) + 1);
    if (((stackitem) R0) == ((stackitem) R1)) {
        rstk -= 3;		      /* Pop iteration variable and limit */
        ip++;			      /* Skip the jump address */
    } else {
        ip += (stackitem) *ip;
    }
}

prim P_xploop() 		      /* Execute +LOOP */
{
    stackitem niter;

    Sl(1);
    Rsl(3);

    niter = ((stackitem) R0) + S0;
    Pop;
    if ((niter >= ((stackitem) R1)) &&
            (((stackitem) R0) < ((stackitem) R1))) {
        rstk -= 3;		      /* Pop iteration variable and limit */
        ip++;			      /* Skip the jump address */
    } else {
        ip += (stackitem) *ip;
        R0 = (rstackitem) niter;
    }
}

prim P_leave()			      /* Compile LEAVE */
{
    Rsl(3);
    ip = R2;
    rstk -= 3;
}

prim P_i()			      /* Obtain innermost loop index */
{
    Rsl(3);
    So(1);
    Push = (stackitem) R0;            /* It's the top item on return stack */
}

prim P_j()			      /* Obtain next-innermost loop index */
{
    Rsl(6);
    So(1);
    Push = (stackitem) rstk[-4];      /* It's the 4th item on return stack */
}

prim P_quit()			      /* Terminate execution */
{
    rstk = rstack;		      /* Clear return stack */
#ifdef WALKBACK
    wbptr = wback;
#endif
    ip = NULL;			      /* Stop execution of current word */
}

prim P_abort()			      /* Abort, clearing data stack */
{
    Sl(1);
    So(0);
    char buffer[16];

    int flag=(int)S0;
    Pop;

    if ( flag != 0 ) {
        sprintf(buffer,"Aborting\n");
#ifdef MBED
        atlastTxString(buffer);
#endif
        P_clear();			      /* Clear the data stack */
        pwalkback();
        P_quit();			      /* Shut down execution */

    }
}

/* Abort, printing message */
prim P_abortq() {
    char buffer[80];
    if (state) {
        stringlit = True;	      /* Set string literal expected */
        Compconst(s_abortq);	      /* Compile ourselves */
    } else {
        /* Otherwise, print string literal in in-line code. */

        sprintf(buffer,"%s", ((char *) ip) + 1);  // EMBEDDED
#ifdef MBED
        atlastTxString(buffer);
#endif

#ifdef WALKBACK
        pwalkback();
#endif /* WALKBACK */
        P_abort();		      /* Abort */
        atl_comment = state = Falsity;/* Reset all interpretation state */
        forgetpend = defpend = stringlit =
            tickpend = ctickpend = False;
    }
}

/*  Compilation primitives  */

prim P_immediate()		      /* Mark most recent word immediate */
{
    dict->wname[0] |= IMMEDIATE;
}

prim P_lbrack() 		      /* Set interpret state */
{
    Compiling;
    state = Falsity;
}

prim P_rbrack() 		      /* Restore compile state */
{
    state = Truth;
}

Exported void P_dodoes()	      /* Execute indirect call on method */
{
    Rso(1);
    So(1);
    Rpush = ip; 		      /* Push instruction pointer */
#ifdef WALKBACK
    *wbptr++ = curword; 	      /* Place word on walkback stack */
#endif
    /* The compiler having craftily squirreled away the DOES> clause
    address before the word definition on the heap, we back up to
    the heap cell before the current word and load the pointer from
    there.  This is an ABSOLUTE heap address, not a relative offset. */
    ip = *((dictword ***) (((stackitem *) curword) - 1));

    /* Push the address of this word's body as the argument to the
    DOES> clause. */
    Push = (stackitem) (((stackitem *) curword) + Dictwordl);
}

prim P_does()			      /* Specify method for word */
{

    /* O.K., we were compiling our way through this definition and we've
    encountered the Dreaded and Dastardly Does.  Here's what we do
    about it.  The problem is that when we execute the word, we
    want to push its address on the stack and call the code for the
    DOES> clause by diverting the IP to that address.  But...how
    are we to know where the DOES> clause goes without adding a
    field to every word in the system just to remember it.  Recall
    that since this system is portable we can't cop-out through
    machine code.  Further, we can't compile something into the
    word because the defining code may have already allocated heap
    for the word's body.  Yukkkk.  Oh well, how about this?  Let's
    copy any and all heap allocated for the word down one stackitem
    and then jam the DOES> code address BEFORE the link field in
    the word we're defining.

    Then, when (DOES>) (P_dodoes) is called to execute the word, it
    will fetch that code address by backing up past the start of
    the word and seting IP to it.  Note that FORGET must recognise
    such words (by the presence of the pointer to P_dodoes() in
    their wcode field, in case you're wondering), and make sure to
    deallocate the heap word containing the link when a
    DOES>-defined word is deleted.  */

    if (createword != NULL) {
        stackitem *sp = ((stackitem *) createword), *hp;

        Rsl(1);
        Ho(1);

        /* Copy the word definition one word down in the heap to
        permit us to prefix it with the DOES clause address. */

        for (hp = hptr - 1; hp >= sp; hp--)
            *(hp + 1) = *hp;
        hptr++; 		      /* Expand allocated length of word */
        *sp++ = (stackitem) ip;       /* Store DOES> clause address before
                                        word's definition structure. */
        createword = (dictword *) sp; /* Move word definition down 1 item */
        createword->wcode = P_dodoes; /* Set code field to indirect jump */

        /* Now simulate an EXIT to bail out of the definition without
        executing the DOES> clause at definition time. */

        ip = R0;		      /* Set IP to top of return stack */
#ifdef WALKBACK
        wbptr = (wbptr > wback) ? wbptr - 1 : wback;
#endif
        Rpop;			      /* Pop the return stack */
    }
}

prim P_colon()			      /* Begin compilation */
{
    state = Truth;		      /* Set compilation underway */
    P_create(); 		      /* Create conventional word */
}

prim P_semicolon()		      /* End compilation */
{
    Compiling;
    Ho(1);
    Hstore = s_exit;
    state = Falsity;		      /* No longer compiling */
    /* We wait until now to plug the P_nest code so that it will be
    present only in completed definitions. */
    if (createword != NULL)
        createword->wcode = P_nest;   /* Use P_nest for code */
    createword = NULL;		      /* Flag no word being created */
}

prim ATH_char() {
    int i;
    char c;

    i=token(&instream);

    if( i != TokNull) {
        c=tokbuf[0];
        Push=c;
    }
}
/* Take address of next word */
prim P_tick() {
    int i;
    char buffer[132];

    /* Try to get next symbol from the input stream.  If
    we can't, and we're executing a compiled word,
    report an error.  Since we can't call back to the
    calling program for more input, we're stuck. */

    i = token(&instream);	      /* Scan for next token */
    if (i != TokNull) {
        if (i == TokWord) {
            dictword *di;

            ucase(tokbuf);
            if ((di = lookup(tokbuf)) != NULL) {
                So(1);
                Push = (stackitem) di; /* Push word compile address */
            } else {
                sprintf(buffer," '%s' undefined ", tokbuf); // EMBEDDED
#ifdef MBED
                atlastTxString(buffer);
#endif
            }
        } else {

            sprintf(buffer,"\nWord not specified when expected.\n"); // EMBEDDED
#ifdef MBED
            atlastTxString(buffer);
#endif
            P_abort();
        }
    } else {
        /* O.K., there was nothing in the input stream.  Set the
        tickpend flag to cause the compilation address of the next
        token to be pushed when it's supplied on a subsequent input
        line. */
        if (ip == NULL) {
            tickpend = True;	      /* Set tick pending */
        } else {

            sprintf(buffer,"\nWord requested by ` not on same input line.\n");
#ifdef MBED
            atlastTxString(buffer);
#endif

            P_abort();
        }
    }
}

prim P_bracktick()		      /* Compile in-line code address */
{
    Compiling;
    ctickpend = True;		      /* Force literal treatment of next
                                    word in compile stream */
}

prim P_execute()		      /* Execute word pointed to by stack */
{
    dictword *wp;

    Sl(1);
    wp = (dictword *) S0;	      /* Load word address from stack */
    Pop;			      /* Pop data stack before execution */
    exword(wp); 		      /* Recursively call exword() to run
                                the word. */
}

prim P_body()			      /* Get body address for word */
{
    Sl(1);
    S0 += Dictwordl * sizeof(stackitem);
}

prim P_state()			      /* Get state of system */
{
    So(1);
    Push = (stackitem) &state;
}

/*  Definition field access primitives	*/

#ifdef DEFFIELDS

prim P_find()			      /* Look up word in dictionary */
{
    dictword *dw;

    Sl(1);
    So(1);
    Hpc(S0);
    V strcpy(tokbuf, (char *) S0);    /* Use built-in token buffer... */
    dw = lookup(tokbuf);              /* So ucase() in lookup() doesn't wipe */
    /* the token on the stack */
    if (dw != NULL) {
        S0 = (stackitem) dw;
        /* Push immediate flag */
        Push = (dw->wname[0] & IMMEDIATE) ? 1 : -1;
    } else {
        Push = 0;
    }
}

#define DfOff(fld)  (((char *) &(dict->fld)) - ((char *) dict))

prim P_toname() 		      /* Find name field from compile addr */
{
    Sl(1);
    S0 += DfOff(wname);
}

    /* Find link field from compile addr */
prim P_tolink() {
    char buffer[80];

    if (DfOff(wnext) != 0) {
        sprintf(buffer,"\n>LINK Foulup--wnext is not at zero!\n");
#ifdef MBED
        atlastTxString(buffer);
#endif
    }
    /*  Sl(1);
        S0 += DfOff(wnext);  */	      /* Null operation.  Wnext is first */
}

prim P_frombody()		      /* Get compile address from body */
{
    Sl(1);
    S0 -= Dictwordl * sizeof(stackitem);
}

prim P_fromname()		      /* Get compile address from name */
{
    Sl(1);
    S0 -= DfOff(wname);
}

/* Get compile address from link */
prim P_fromlink() {
    char buffer[40];

    if (DfOff(wnext) != 0) {
        sprintf(buffer,"\nLINK> Foulup--wnext is not at zero!\n");
#ifdef MBED
        atlastTxString(buffer);
#endif
    }
    /*  Sl(1);
        S0 -= DfOff(wnext);  */	      /* Null operation.  Wnext is first */
}

#undef DfOff

#define DfTran(from,to) (((char *) &(dict->to)) - ((char *) &(dict->from)))

prim P_nametolink()		      /* Get from name field to link */
{
    char *from, *to;

    Sl(1);
    /*
    S0 -= DfTran(wnext, wname);
    */
    from = (char *) &(dict->wnext);
    to = (char *) &(dict->wname);
    S0 -= (to - from);
}

prim P_linktoname()		      /* Get from link field to name */
{
    char *from, *to;

    Sl(1);
    /*
    S0 += DfTran(wnext, wname);
    */
    from = (char *) &(dict->wnext);
    to = (char *) &(dict->wname);
    S0 += (to - from);
}

prim P_fetchname()		      /* Copy word name to string buffer */
{
    Sl(2);			      /* nfa string -- */
    Hpc(S0);
    Hpc(S1);
    /* Since the name buffers aren't in the system heap, but
    rather are separately allocated with alloc(), we can't
    check the name pointer references.  But, hey, if the user's
    futzing with word dictionary items on the heap in the first
    place, there's a billion other ways to bring us down at
    his command. */
    V strcpy((char *) S0, *((char **) S1) + 1);
    Pop2;
}

prim P_storename()		      /* Store string buffer in word name */
{
    char tflags;
    char *cp;

    Sl(2);			      /* string nfa -- */
    Hpc(S0);			      /* See comments in P_fetchname above */
    Hpc(S1);			      /* checking name pointers */
    tflags = **((char **) S0);
    free(*((char **) S0));
    *((char **) S0) = cp = alloc((unsigned int) (strlen((char *) S1) + 2));
    V strcpy(cp + 1, (char *) S1);
    *cp = tflags;
    Pop2;
}

#endif /* DEFFIELDS */

#ifdef SYSTEM
prim P_system()
{				      /* string -- status */
    Sl(1);
    Hpc(S0);
    S0 = system((char *) S0);
}

// Time to leave.
#endif /* SYSTEM */

#ifdef TRACE
prim P_trace()			      /* Set or clear tracing of execution */
{
    Sl(1);
    atl_trace = (S0 == 0) ? Falsity : Truth;
    Pop;
}
#endif /* TRACE */

#ifdef WALKBACK
prim P_walkback()		      /* Set or clear error walkback */
{
    Sl(1);
    atl_walkback = (S0 == 0) ? Falsity : Truth;
    Pop;
}
#endif /* WALKBACK */

#ifdef WORDSUSED

    /* List words used by program */
prim P_wordsused() {
    dictword *dw = dict;

    char buffer[80];

    while (dw != NULL) {
        if (*(dw->wname) & WORDUSED) {
            sprintf("\r\n%s", dw->wname + 1); // EMBEDDED
#ifdef MBED
            atlastTxString(buffer);
#endif
        }
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#endif
        dw = dw->wnext;
    }

#ifdef MBED
        atlastTxByte('\n');
#endif

}

prim P_wordsunused()		      /* List words not used by program */
{
    dictword *dw = dict;

    char buffer[80];

    while (dw != NULL) {
        if (!(*(dw->wname) & WORDUSED)) {
            sprintf(buffer,"\r\n%s", dw->wname + 1);
#ifdef MBED
            atlastTxString(buffer);
#endif
        }
#ifdef Keyhit
        if (kbquit()) {
            break;
        }
#endif
        dw = dw->wnext;
    }

#ifdef MBED
        atlastTxByte('\n');
#endif

}
#endif /* WORDSUSED */

#ifdef COMPILERW

prim P_brackcompile()		      /* Force compilation of immediate word */
{
    Compiling;
    cbrackpend = True;		      /* Set [COMPILE] pending */
}

prim P_literal()		      /* Compile top of stack as literal */
{
    Compiling;
    Sl(1);
    Ho(2);
    Hstore = s_lit;		      /* Compile load literal word */
    Hstore = S0;		      /* Compile top of stack in line */
    Pop;
}

prim P_compile()		      /* Compile address of next inline word */
{
    Compiling;
    Ho(1);
    Hstore = (stackitem) *ip++;       /* Compile the next datum from the
                                        instruction stream. */
}

prim P_backmark()		      /* Mark backward backpatch address */
{
    Compiling;
    So(1);
    Push = (stackitem) hptr;	      /* Push heap address onto stack */
}

prim P_backresolve()		      /* Emit backward jump offset */
{
    stackitem offset;

    Compiling;
    Sl(1);
    Ho(1);
    Hpc(S0);
    offset = -(hptr - (stackitem *) S0);
    Hstore = offset;
    Pop;
}

prim P_fwdmark()		      /* Mark forward backpatch address */
{
    Compiling;
    Ho(1);
    Push = (stackitem) hptr;	      /* Push heap address onto stack */
    Hstore = 0;
}

prim P_fwdresolve()		      /* Emit forward jump offset */
{
    stackitem offset;

    Compiling;
    Sl(1);
    Hpc(S0);
    offset = (hptr - (stackitem *) S0);
    *((stackitem *) S0) = offset;
    Pop;
}

#endif /* COMPILERW */

/*  Table of primitive words  */

static struct primfcn primt[] = {
    {"0+", P_plus},
    {"0-", P_minus},
    {"0*", P_times},
    {"0/", P_div},
    {"0MOD", P_mod},
    {"0/MOD", P_divmod},
    {"0MIN", P_min},
    {"0MAX", P_max},
    {"0NEGATE", P_neg},
    {"0ABS", P_abs},
    {"0=", P_equal},
    {"0<>", P_unequal},
    {"0>", P_gtr},
    {"0<", P_lss},
    {"0>=", P_geq},
    {"0<=", P_leq},
    {"0BETWEEN", P_between},
    {"0LSTRIP", P_lstrip},

    {"0AND", P_and},
    {"0OR", P_or},
    {"0XOR", P_xor},
    {"0NOT", P_not},
    {"0SHIFT", P_shift},

    {"0DEPTH", P_depth},
    {"0CLEAR", P_clear},
    {"0DUP", P_dup},
    {"0DROP", P_drop},
    {"0SWAP", P_swap},
    {"0NIP", P_nip},
    {"0OVER", P_over},
    {"0PICK", P_pick},
    {"0ROT", P_rot},
    {"0-ROT", P_minusrot},
    {"0ROLL", P_roll},
    {"0>R", P_tor},
    {"0R>", P_rfrom},
    {"0R@", P_rfetch},
    {"0TOKEN", ATH_Token},
    {"0ERRNO", ATH_errno},

#ifdef SHORTCUTA
    {"01+", P_1plus},
    {"02+", P_2plus},
    {"01-", P_1minus},
    {"02-", P_2minus},
    {"02*", P_2times},
    {"02/", P_2div},
#endif /* SHORTCUTA */

#ifdef SHORTCUTC
    {"00=", P_0equal},
    {"00<>", P_0notequal},
    {"00>", P_0gtr},
    {"00<", P_0lss},
#endif /* SHORTCUTC */

#ifdef DOUBLE
    {"02DUP", P_2dup},
    {"02DROP", P_2drop},
    {"02SWAP", P_2swap},
    {"02OVER", P_2over},
    {"02ROT", P_2rot},
    {"02VARIABLE", P_2variable},
    {"02CONSTANT", P_2constant},
    {"02!", P_2bang},
    {"02@", P_2at},
#endif /* DOUBLE */

    {"0VARIABLE", P_variable},
    {"0CONSTANT", P_constant},
    {"0!", P_bang},
    {"0@", P_at},
    {"0+!", P_plusbang},
    {"0ALLOT", P_allot},
    {"0,", P_comma},
    {"0C!", P_cbang},
    {"0C@", P_cat},
    {"0C,", P_ccomma},
    {"0C=", P_cequal},
    {"0HERE", P_here},

#ifdef ARRAY
    {"0ARRAY", P_array},
#endif

#ifdef STRING
    {"0(STRLIT)", P_strlit},
    {"0STRING", P_string},
    {"0STRCPY", P_strcpy},
    {"0S!", P_strcpy},
    {"0STRCAT", P_strcat},
    {"0S+", P_strcat},
    {"0STRLEN", P_strlen},
    {"0STRCMP", P_strcmp},
    {"0STRCHAR", P_strchar},
    {"0SUBSTR", P_substr},
    {"0COMPARE", P_strcmp},
    {"0STRFORM", P_strform},
    {"0MOVE", P_move},
    {"0STRSEP", P_strsep},
#ifdef REAL
    {"0FSTRFORM", P_fstrform},
    {"0STRREAL", P_strreal},
#endif
    {"0STRINT", P_strint},
#endif /* STRING */

#ifdef REAL
    {"0(FLIT)", P_flit},
    {"0F+", P_fplus},
    {"0F-", P_fminus},
    {"0F*", P_ftimes},
    {"0F/", P_fdiv},
    {"0FMIN", P_fmin},
    {"0FMAX", P_fmax},
    {"0FNEGATE", P_fneg},
    {"0FABS", P_fabs},
    {"0F=", P_fequal},
    {"0F<>", P_funequal},
    {"0F>", P_fgtr},
    {"0F<", P_flss},
    {"0F>=", P_fgeq},
    {"0F<=", P_fleq},
    {"0F.", P_fdot},
    {"0FLOAT", P_float},
    {"0FIX", P_fix},
#ifdef MATH
    {"0ACOS", P_acos},
    {"0ASIN", P_asin},
    {"0ATAN", P_atan},
    {"0ATAN2", P_atan2},
    {"0COS", P_cos},
    {"0EXP", P_exp},
    {"0LOG", P_log},
    {"0POW", P_pow},
    {"0SIN", P_sin},
    {"0SQRT", P_sqrt},
    {"0TAN", P_tan},
#endif /* MATH */
#endif /* REAL */

    {"0(NEST)", P_nest},
    {"0EXIT", P_exit},
    {"0(LIT)", P_dolit},
    {"0BRANCH", P_branch},
    {"0?BRANCH", P_qbranch},
    {"1IF", P_if},
    {"1ELSE", P_else},
    {"1THEN", P_then},
    {"0?DUP", P_qdup},
    {"1BEGIN", P_begin},
    {"1UNTIL", P_until},
    {"1AGAIN", P_again},
    {"1WHILE", P_while},
    {"1REPEAT", P_repeat},
    {"1DO", P_do},
    {"1?DO", P_qdo},
    {"1LOOP", P_loop},
    {"1+LOOP", P_ploop},
    {"0(XDO)", P_xdo},
    {"0(X?DO)", P_xqdo},
    {"0(XLOOP)", P_xloop},
    {"0(+XLOOP)", P_xploop},
    {"0LEAVE", P_leave},
    {"0I", P_i},
    {"0J", P_j},
    {"0QUIT", P_quit},
    {"0ABORT", P_abort},
    {"1ABORT\"", P_abortq},

#ifdef SYSTEM
    {"0SYSTEM", P_system},
//    {"0BYE", ATH_bye},
#endif
#ifdef TRACE
    {"0TRACE", P_trace},
#endif
#ifdef WALKBACK
    {"0WALKBACK", P_walkback},
#endif

#ifdef WORDSUSED
    {"0WORDSUSED", P_wordsused},
    {"0WORDSUNUSED", P_wordsunused},
#endif

#ifdef MEMSTAT
    {"0MEMSTAT", atl_memstat},
#endif

    {"0:", P_colon},
    {"1;", P_semicolon},
    {"0IMMEDIATE", P_immediate},
    {"1[", P_lbrack},
    {"0]", P_rbrack},
    {"0CREATE", P_create},
    {"0FORGET", P_forget},
    {"0DOES>", P_does},
    {"0'", P_tick},
    {"1[']", P_bracktick},
    {"0EXECUTE", P_execute},
    {"0>BODY", P_body},
    {"0STATE", P_state},
    {"0CHAR", ATH_char},

#ifdef DEFFIELDS
    {"0FIND", P_find},
    {"0>NAME", P_toname},
    {"0>LINK", P_tolink},
    {"0BODY>", P_frombody},
    {"0NAME>", P_fromname},
    {"0LINK>", P_fromlink},
    {"0N>LINK", P_nametolink},
    {"0L>NAME", P_linktoname},
    {"0NAME>S!", P_fetchname},
    {"0S>NAME!", P_storename},
#endif /* DEFFIELDS */

#ifdef COMPILERW
    {"1[COMPILE]", P_brackcompile},
    {"1LITERAL", P_literal},
    {"0COMPILE", P_compile},
    {"0<MARK", P_backmark},
    {"0<RESOLVE", P_backresolve},
    {"0>MARK", P_fwdmark},
    {"0>RESOLVE", P_fwdresolve},
#endif /* COMPILERW */

#ifdef CONIO
    {"0.", P_dot},
    {"0?", P_question},
    {"0CR", P_cr},
    {"0.S", P_dots},
    {"1.\"", P_dotquote},
    {"1.(", P_dotparen},
    {"0TYPE", P_type},
    {"0WORDS", P_words},
    {"0$SIFT", ATH_sift},
    {"0EMIT", P_emit},
#endif /* CONIO */
#ifdef LINUX
    {"0FD-READ", P_fdRead},
    {"0FD-WRITE", P_fdWrite},
    {"0STRSTR", P_strstr},
    {"0STRCASESTR", P_strcasestr},
#endif

#ifdef LIBSER
    {"0OPEN-SERIAL-PORT", ATH_openSerialPort},
    {"0CLOSE-SERIAL-PORT", ATH_closeSerialPort},
    {"0FLUSH-SERIAL-PORT", ATH_flushSerialPort},
    {"0?BLOCK", ATH_wouldBlock},
#endif

#ifdef FILEIO
    {"0FILE", P_file},
    {"0(MKDIR)", P_mkdir},
    {"0(RMDIR)", P_rmdir},

//    {"0FACCESS", P_fopen},
//    {"0FOPEN", P_fopen},
    {"0ACCESS", P_access},
//    {"0FCLOSE", P_fclose},
    {"0UNLINK", P_fdelete},
    {"0FGETS", P_fgetline},
    {"0FPUTS", P_fputline},
//    {"0FREAD", P_fread},
//    {"0FWRITE", P_fwrite},
    {"0FGETC", P_fgetc},
    {"0FPUTC", P_fputc},
    {"0FTELL", P_ftell},
//    {"0FSEEK", P_fseek},
    {"0FLOAD", P_fload},
    {"0$INCLUDE", P_include},
    {(char *)"0PWD", ATH_pwd},
    {(char *)"0CD", ATH_cd},
    {(char *)"0DIR", RT_dir},
    {(char *)"0TOUCH", RT_touch},
    {(char *)"0MKFILE", RT_mkfile},
    {(char *)"0CRCFILE", RT_crcfile},
    {(char *)"0TEST", RT_test},
#endif /* FILEIO */

#ifdef EVALUATE
    {"0EVALUATE", P_evaluate},
#endif /* EVALUATE */

#ifdef ATH
#ifdef SOCKETS
        {(char *)"0SOCKET-CONNECT",athConnect},
        {(char *)"0SOCKET-CLOSE",athClose},
        {(char *)"0SOCKET-SEND",athSend},
        {(char *)"0SOCKET-RECV",athRecv},
        {(char *)"0ADD-EOL",athAddEOL},
        {(char *)"0CMD-GET",athCmdGet},
    {(char *)"0CMD-SET",athCmdSet},
#endif
    {(char *)"0ON",ATH_on},
    {(char *)"0OFF",ATH_off},
    {(char *)"0MKBUFFER",ATH_mkBuffer},
    {(char *)"0MEMSAFE", ATH_memsafe},
    {(char *)"0?MEMSAFE",ATH_qmemsafe},
    {(char *)"0DUMP",ATH_dump},
    {(char *)"0FILL",ATH_fill},
    {(char *)"0ERASE",ATH_erase},
    {(char *)"0W@",ATH_wat},
    {(char *)"0W!",ATH_wbang},
    {(char *)"0W>CELL",ATH_16toCell},

    {(char *)"0HEX",ATH_hex},
    {(char *)"0DECIMAL",ATH_dec},
    {(char *)"0BYE",ATH_bye},
    {(char *)"0?FILEIO",ATH_qfileio},
    {(char *)"0TIB", ATH_Instream},
//    {(char *)"0TOKEN", ATH_Token},
    {(char *)"0?LINUX", ATH_qlinux},
    {(char *)"0?FREERTOS", ATH_qfreertos},
//    {(char *)"0MS", ATH_ms},

#endif
    {(char *)"0.FEATURES", ATH_Features},
    {(char *)"0PERROR", ATH_perror},

#ifdef ANSI
    {(char *)"0CELL", ANSI_cell},
    {(char *)"0CELLS", ANSI_cells},
    {(char *)"0CELL+", ANSI_cellplus},
    {(char *)"0CHARS", ANSI_chars},
    {(char *)"0ALLOCATE", ANSI_allocate},
    {(char *)"0FREE", ANSI_free},
#endif
#ifdef FREERTOS
    {(char *)"0TOUCH", RT_touch},
    {(char *)"0MKFILE", RT_mkfile},
    {(char *)"0CRCFILE", RT_crcfile},
    {(char *)"0TEST", RT_test},

        {(char *)"0MKSCMD", FR_mkScmd},
        {(char *)"0NAND-SETUP", FR_NANDSetup},
        {(char *)"0NAND-BLOCKS", FR_NANDBlocks},
        {(char *)"0NAND-MKBAD", FR_badBlockMap },
        {(char *)"0NAND-LOCK", FR_NANDLock},
        {(char *)"0NAND-UNLOCK", FR_NANDUnlock },
        {(char *)"0NAND-READ", FR_NANDRead },
        {(char *)"0NAND-WRITE", FR_NANDWrite },
        {(char *)"0NAND-ERASE", FR_NANDErase },
        {(char *)"0NAND-GETPARAMS", FR_NANDGetParams },

#ifdef YAFFS
        {(char *)"0YAFFS-INSTALL", FR_yaffsInstall },
        {(char *)"0YAFFS-FORMAT", FR_yaffsFormat },
        {(char *)"0YAFFS-MOUNT", FR_yaffsMount },
        {(char *)"0YAFFS-UNMOUNT", FR_yaffsUnmount },
        {(char *)"0O_CREAT", FR_yaffsCreatDef },
        {(char *)"0O_TRUNC", FR_yaffsTruncDef },
        {(char *)"0O_RDWR", FR_yaffsRdwrDef },
        {(char *)"0O_RDONLY", FR_yaffsRdOnly },
        {(char *)"0O_WRONLY", FR_yaffsWrOnly },
        {(char *)"0O_APPEND", FR_yaffsAppend },

        {(char *)"0SEEK_SET", FR_yaffsSeekSet },
        {(char *)"0SEEK_CUR", FR_yaffsSeekCur },
        {(char *)"0SEEK_END", FR_yaffsSeekEnd },
        {(char *)"0LOAD-FILE", FR_loadFile },

#endif

        {(char *)"0RX", FR_rxXmodem},
    {(char *)"0QID@", FR_getQid},
    {(char *)"0DB@", FR_getTaskDb},
    {(char *)"0DB!", FR_setTaskDb},

        {(char *)"0WRITE-PIN", FR_writePin},
        {(char *)"0READ-PIN", FR_readPin},
        {(char *)"0SET-BACKLIGHT", FR_setBacklight},

    {(char *)"0POOL@", FR_getPoolId } ,
    {(char *)"0POOL-FREE", FR_poolFree } ,
    {(char *)"0POOL-ALLOCATE", FR_poolAllocate } ,

    {(char *)"0CMD-PARSE", FR_cmdParse },
    {(char *)"0TICK@", FR_getSysTick },
        {(char *)"0LCD-RESET", FR_lcdReset },
        {(char *)"0LCD-REG-SET",FR_lcdRegSet},

        {(char *)"0DEV-RESET", FR_devReset},
        {(char *)"0?UART-RX", FR_uartRxReady},
        {(char *)"0UART-TYPE", FR_uartTxBuffer},
        {(char *)"0UART-KEY", FR_uartRxByte},
        {(char *)"0UART-READLINE",FR_uartReadLine},
        {(char *)"0UART-EMIT",  FR_uartEmit},
        {(char *)"0RING-BUFFER@", FR_ringBufferPtr },
        {(char *)"0RING-BUFFER-ERASE", FR_ringBufferErase },

        {(char *)"0?KEY", FR_qconsoleKey },
        {(char *)"0KEY", FR_consoleKey },
        {(char *)"0EXPECT", FR_consoleExpect },

        {(char *)"0SEM@", FR_semGet },
        {(char *)"0SEM-COUNT@", FR_semGetCount },
        {(char *)"0SEM-GIVE", FR_semGive },
        {(char *)"0SEM-TAKE", FR_semTake },
        {(char *)"0STACK-HWM", FR_stackHighWaterMark },
        {(char *)"0PS", FR_ps },
        {(char *)"0SUSPEND", FR_suspend },
        {(char *)"0RESUME", FR_resume },
        {(char *)"0NICE", FR_nice },
        {(char *)"0GET-TASK-HANDLE", FR_getTaskHandle },
        {(char *)"0GET-TASK-STATE", FR_getTaskState },

#endif
#ifdef PUBSUB
//    {(char *)"0MKDB",     FR_mkdb},
    {(char *)"0ADD-RECORD",  FR_addRecord},
    {(char *)"0LOOKUP",  FR_lookup},
    {(char *)"0LOOKUP-REC",  FR_lookupRecord},
    {(char *)"0PUBLISH",  FR_publish},
    {(char *)"0GET-SUBCOUNT",  FR_subCount},
    {(char *)"0.RECORD",  FR_displayRecord},
    {(char *)"0MESSAGE@", FR_getMessage},
    {(char *)"0MESSAGE!", FR_putMessage},
#ifdef FREERTOS
    {(char *)"0EVENT@", FR_getEvent},
    // This code is compiled if PUBSUB AND FREERTOS are defined
    {(char *)"0MKMSG-GET", FR_mkmsgGet},
    {(char *)"0MKMSG-SET", FR_mkmsgSet},
    {(char *)"0MKMSG-SUB", FR_mkmsgSub},
    {(char *)"0MKMSG-UNSUB", FR_mkmsgUnsub},

    {(char *)"0MKMSG-OPEN", FR_mkmsgOpen},

    {(char *)"0SET-CMD", FR_setCmd},
    {(char *)"0GET-CMD", FR_getCmd},

    {(char *)"0SET-KEY", FR_setKey},
    {(char *)"0GET-KEY", FR_getKey},

    {(char *)"0SET-VALUE", FR_setValue},
    {(char *)"0GET-VALUE", FR_getValue},

    {(char *)"0SET-FIELD-CNT", FR_setFieldCnt},
    {(char *)"0GET-FIELD-CNT", FR_getFieldCnt},

    {(char *)"0SET-SENDER", FR_setSender},
    {(char *)"0GET-SENDER", FR_getSender},
#endif
#ifdef PTHREAD
        // TODO Rename all in this section from FR_ to PS_
    {(char *)"0COMMS", PS_comms},
#endif

#endif
    {NULL, (codeptr) 0}
};

/*  ATL_PRIMDEF  --  Initialise the dictionary with the built-in primitive
    words.  To save the memory overhead of separately
    allocated word items, we get one buffer for all
    the items and link them internally within the buffer. */

void atl_primdef( struct primfcn *pt) {
    struct primfcn *pf = pt;
    dictword *nw;
    int i, n = 0;
#ifdef WORDSUSED
#ifdef READONLYSTRINGS
    unsigned int nltotal;
    char *dynames, *cp;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */

    /* Count the number of definitions in the table. */

    while (pf->pname != NULL) {
        n++;
        pf++;
    }

#ifdef WORDSUSED
#ifdef READONLYSTRINGS
    nltotal = n;
    for (i = 0; i < n; i++) {
        nltotal += strlen(pt[i].pname);
    }
    cp = dynames = alloc(nltotal);
    for (i = 0; i < n; i++) {
        strcpy(cp, pt[i].pname);
        cp += strlen(cp) + 1;
    }
    cp = dynames;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */

    nw = (dictword *) alloc((unsigned int) (n * sizeof(dictword)));

    nw[n - 1].wnext = dict;
    dict = nw;
    for (i = 0; i < n; i++) {
        nw->wname = pt->pname;
#ifdef WORDSUSED
#ifdef READONLYSTRINGS
        nw->wname = cp;
        cp += strlen(cp) + 1;
#endif /* READONLYSTRINGS */
#endif /* WORDSUSED */
        nw->wcode = pt->pcode;
        if (i != (n - 1)) {
            nw->wnext = nw + 1;
        }
        nw++;
        pt++;
    }
}

#ifdef WALKBACK

/*  PWALKBACK  --  Print walkback trace.  */

static void pwalkback() {
    char buffer[80];

        if (atl_walkback && ((curword != NULL) || (wbptr > wback))) {

                sprintf(buffer,"Walkback:\r\n"); // EMBEDDED
#ifdef MBED
        atlastTxString(buffer);
#endif
                if (curword != NULL) {
                        sprintf(buffer,"   %s\r\n", curword->wname + 1); // EMBEDDED
#ifdef MBED
        atlastTxString(buffer);
#endif
                }
                while (wbptr > wback) {
                        dictword *wb = *(--wbptr);

                        sprintf(buffer,"   %s\r\n", wb->wname + 1); // EMBEDDED
#ifdef MBED
        atlastTxString(buffer);
#endif
                }
        }
}
#endif /* WALKBACK */

/*  TROUBLE  --  Common handler for serious errors.  */

// static void trouble(char *kind) {
void trouble(char *kind) {
#ifdef MEMMESSAGE
    char buffer[80];
    sprintf(buffer,"\n%s.\r\n", kind); // EMBEDDED
#ifdef MBED
    atlastTxString(buffer);
#endif
#endif
#ifdef WALKBACK
    pwalkback();
#endif /* WALKBACK */
    P_abort();			      /* Abort */
    atl_comment = state = Falsity;    /* Reset all interpretation state */
    forgetpend = defpend = stringlit =
        tickpend = ctickpend = False;
}

/*  ATL_ERROR  --  Handle error detected by user-defined primitive.  */

void atl_error(char *kind) {
    trouble(kind);
    evalstat = ATL_APPLICATION;       /* Signify application-detected error */
}

#ifndef NOMEMCHECK

/*  STAKOVER  --  Recover from stack overflow.	*/

void stakover() {
    trouble("Stack overflow");
    evalstat = ATL_STACKOVER;
}

/*  STAKUNDER  --  Recover from stack underflow.  */

void stakunder()
{
    trouble("Stack underflow");
    evalstat = ATL_STACKUNDER;
}

/*  RSTAKOVER  --  Recover from return stack overflow.	*/

void rstakover()
{
    trouble("Return stack overflow");
    evalstat = ATL_RSTACKOVER;
}

/*  RSTAKUNDER	--  Recover from return stack underflow.  */

void rstakunder()
{
    trouble("Return stack underflow");
    evalstat = ATL_RSTACKUNDER;
}

/*  HEAPOVER  --  Recover from heap overflow.  Note that a heap
    overflow does NOT wipe the heap; it's up to
    the user to do this manually with FORGET or
    some such. */

void heapover()
{
    trouble("Heap overflow");
    evalstat = ATL_HEAPOVER;
}

/*  BADPOINTER	--  Abort if bad pointer reference detected.  */

void badpointer()
{
    trouble("Bad pointer");
    evalstat = ATL_BADPOINTER;
}

/*  NOTCOMP  --  Compiler word used outside definition.  */

static void notcomp()
{
    trouble("Compiler word outside definition");
    evalstat = ATL_NOTINDEF;
}

/*  DIVZERO  --  Attempt to divide by zero.  */

static void divzero()
{
    trouble("Divide by zero");
    evalstat = ATL_DIVZERO;
}

#endif /* !NOMEMCHECK */

/*  EXWORD  --	Execute a word (and any sub-words it may invoke). */

// static void exword( dictword *wp) {
void exword( dictword *wp) {
    char buffer[80];
    curword = wp;
#ifdef TRACE
    if (atl_trace) {

        P_dots();
#ifdef MBED
    atlastTxByte( '\t' );
    atlastTxByte( '\t' );
#endif

        sprintf(buffer,"\nTrace: %s \r\n", curword->wname + 1); //  EMBEDDED
#ifdef MBED
    atlastTxString( buffer );
#endif
    }
#endif /* TRACE */
    (*curword->wcode)();	      /* Execute the first word */
    while (ip != NULL) {
#ifdef BREAK
#ifdef Keybreak
        Keybreak();		      /* Poll for asynchronous interrupt */
#endif
        if (broken) {		      /* Did we receive a break signal */
            trouble("Break signal");
            evalstat = ATL_BREAK;
            break;
        }
#endif /* BREAK */
        curword = *ip++;
#ifdef TRACE
        if (atl_trace) {
            char buffer[80];
            P_dots();
            sprintf(buffer,"\t\t\nTrace: %s ", curword->wname + 1); // EMBEDDED
#ifdef MBED
            atlastTxString( buffer );
#endif
        }
#endif /* TRACE */
        (*curword->wcode)();	      /* Execute the next word */
    }
    curword = NULL;
}

/*  ATL_INIT  --  Initialise the ATLAST system.  The dynamic storage areas
    are allocated unless the caller has preallocated buffers
    for them and stored the addresses into the respective
    pointers.  In either case, the storage management
    pointers are initialised to the correct addresses.  If
    the caller preallocates the buffers, it's up to him to
    ensure that the length allocated agrees with the lengths
    given by the atl_... cells.  */

void atl_init() {
    if (dict == NULL) {
        atl_primdef(primt);	      /* Define primitive words */
        dictprot = dict;	      /* Set protected mark in dictionary */

        /* Look up compiler-referenced words in the new dictionary and
        save their compile addresses in static variables. */

#define Cconst(cell, name)  cell = (stackitem) lookup(name); if(cell==0)abort()
        Cconst(s_exit, "EXIT");
        Cconst(s_lit, "(LIT)");
#ifdef REAL
        Cconst(s_flit, "(FLIT)");
#endif
#ifdef STRING
        Cconst(s_strlit, "(STRLIT)");
#endif
        Cconst(s_dotparen, ".(");
        Cconst(s_qbranch, "?BRANCH");
        Cconst(s_branch, "BRANCH");
        Cconst(s_xdo, "(XDO)");
        Cconst(s_xqdo, "(X?DO)");
        Cconst(s_xloop, "(XLOOP)");
        Cconst(s_pxloop, "(+XLOOP)");
        Cconst(s_abortq, "ABORT\"");
#undef Cconst

        if (stack == NULL) {	      /* Allocate stack if needed */
            stack = (stackitem *)
                alloc(((unsigned int) atl_stklen) * sizeof(stackitem));
        }
        stk = stackbot = stack;
#ifdef MEMSTAT
        stackmax = stack;
#endif
        stacktop = stack + atl_stklen;
        if (rstack == NULL) {	      /* Allocate return stack if needed */
            rstack = (dictword ***)
                alloc(((unsigned int) atl_rstklen) *
                        sizeof(dictword **));
        }
        rstk = rstackbot = rstack;
#ifdef MEMSTAT
        rstackmax = rstack;
#endif
        rstacktop = rstack + atl_rstklen;
#ifdef WALKBACK
        if (wback == NULL) {
            wback = (dictword **) alloc(((unsigned int) atl_rstklen) *
                    sizeof(dictword *));
        }
        wbptr = wback;
#endif
        if (heap == NULL) {

            /* The temporary string buffers are placed at the start of the
            heap, which permits us to pointer-check pointers into them
            as within the heap extents.  Hence, the size of the buffer
            we acquire for the heap is the sum of the heap and temporary
            string requests. */

            int i;
            char *cp;

            /* Force length of temporary strings to even number of
            stackitems. */
            atl_ltempstr += sizeof(stackitem) -
                (atl_ltempstr % sizeof(stackitem));

//            cp = alloc((((unsigned int) atl_heaplen) * sizeof(stackitem)) + ((unsigned int) (atl_ntempstr * atl_ltempstr)));
            int heapSize=((((unsigned int) atl_heaplen) * sizeof(stackitem)) + ((unsigned int) (atl_ntempstr * atl_ltempstr)));
            cp = alloc( heapSize );

            heapbot = (stackitem *) cp;
            strbuf = (char **) alloc(((unsigned int) atl_ntempstr) * sizeof(char *));

            for (i = 0; i < atl_ntempstr; i++) {
                strbuf[i] = cp;
                cp += ((unsigned int) atl_ltempstr);
            }
            cstrbuf = 0;
            heap = (stackitem *) cp;  /* Allocatable heap starts after
                                        the temporary strings */
        }
        /* The system state word is kept in the first word of the heap
        so that pointer checking doesn't bounce references to it.
        When creating the heap, we preallocate this word and initialise
        the state to the interpretive state. */
        hptr = heap + 1;
        state = Falsity;
#ifdef MEMSTAT
        heapmax = hptr;
#endif
        heaptop = heap + atl_heaplen;

        /* Now that dynamic memory is up and running, allocate constants
        and variables built into the system.  */

#ifdef FILEIO
        {   static struct {
                            char *sfn;
                            FILE *sfd;
                        } stdfiles[] = {
                            {"STDIN", NULL},
                            {"STDOUT", NULL},
                            {"STDERR", NULL}
                        };
        int i;
        dictword *dw;

        /* On some systems stdin, stdout, and stderr aren't
        constants which can appear in an initialisation.
        So, we initialise them at runtime here. */

        stdfiles[0].sfd = stdin;
        stdfiles[1].sfd = stdout;
        stdfiles[2].sfd = stderr;

        for (i = 0; i < ELEMENTS(stdfiles); i++) {
            if ((dw = atl_vardef(stdfiles[i].sfn,
                            2 * sizeof(stackitem))) != NULL) {
                stackitem *si = atl_body(dw);
                *si++ = FileSent;
                *si = (stackitem) stdfiles[i].sfd;
            }
        }
        }
#endif /* FILEIO */
        dictprot = dict;	      /* Protect all standard words */
    }
    /*
    rf = atl_vardef("runflag",sizeof(int));
    if( rf != NULL) {
        *((int *) atl_body(rf)) = -1;
    }
    */
}

/*  ATL_LOOKUP	--  Look up a word in the dictionary.  Returns its
    word item if found or NULL if the word isn't
    in the dictionary. */

dictword *atl_lookup( char *name) {
    V strcpy(tokbuf, name);	      /* Use built-in token buffer... */
    ucase(tokbuf);                    /* so ucase() doesn't wreck arg string */
    return lookup(tokbuf);	      /* Now use normal lookup() on it */
}

/*  ATL_BODY  --  Returns the address of the body of a word, given
    its dictionary entry. */

stackitem *atl_body( dictword *dw) {
    return ((stackitem *) dw) + Dictwordl;
}

/*  ATL_EXEC  --  Execute a word, given its dictionary address.  The
    evaluation status for that word's execution is
    returned.  The in-progress evaluation status is
    preserved. */

int atl_exec( dictword *dw) {
    int sestat = evalstat, restat;

    evalstat = ATL_SNORM;
#ifdef BREAK
    broken = False;		      /* Reset break received */
#endif
#undef Memerrs
#define Memerrs evalstat
    Rso(1);
    Rpush = ip; 		      /* Push instruction pointer */
    ip = NULL;			      /* Keep exword from running away */
    exword(dw);
    if (evalstat == ATL_SNORM) {      /* If word ran to completion */
        Rsl(1);
        ip = R0;		      /* Pop the return stack */
        Rpop;
    }
#undef Memerrs
#define Memerrs
    restat = evalstat;
    evalstat = sestat;
    return restat;
}

/*  ATL_VARDEF  --  Define a variable word.  Called with the word's
    name and the number of bytes of storage to allocate
    for its body.  All words defined with atl_vardef()
    have the standard variable action of pushing their
    body address on the stack when invoked.  Returns
    the dictionary item for the new word, or NULL if
    the heap overflows. */

dictword *atl_vardef( char *name, int size) {
    dictword *di;
    int isize = (size + (sizeof(stackitem) - 1)) / sizeof(stackitem);

#undef Memerrs
#define Memerrs NULL
    evalstat = ATL_SNORM;
    Ho(Dictwordl + isize);
#undef Memerrs
#define Memerrs
    if (evalstat != ATL_SNORM)	      /* Did the heap overflow */
        return NULL;		      /* Yes.  Return NULL */
    createword = (dictword *) hptr;   /* Develop address of word */
    createword->wcode = P_var;	      /* Store default code */
    hptr += Dictwordl;		      /* Allocate heap space for word */
    while (isize > 0) {
        Hstore = 0;		      /* Allocate heap area and clear it */
        isize--;
    }
    V strcpy(tokbuf, name);	      /* Use built-in token buffer... */
    ucase(tokbuf);                    /* so ucase() doesn't wreck arg string */
    enter(tokbuf);		      /* Make dictionary entry for it */
    di = createword;		      /* Save word address */
    createword = NULL;		      /* Mark no word underway */
    return di;			      /* Return new word */
}

/*  ATL_MARK  --  Mark current state of the system.  */

void atl_mark( atl_statemark *mp) {
    mp->mstack = stk;		      /* Save stack position */
    mp->mheap = hptr;		      /* Save heap allocation marker */
    mp->mrstack = rstk; 	      /* Set return stack pointer */
    mp->mdict = dict;		      /* Save last item in dictionary */
}

/*  ATL_UNWIND	--  Restore system state to previously saved state.  */

void atl_unwind( atl_statemark *mp) {

    /* If atl_mark() was called before the system was initialised, and
    we've initialised since, we cannot unwind.  Just ignore the
    unwind request.	The user must manually atl_init before an
    atl_mark() request is made. */

    if (mp->mdict == NULL)	      /* Was mark made before atl_init ? */
        return; 		      /* Yes.  Cannot unwind past init */

    stk = mp->mstack;		      /* Roll back stack allocation */
    hptr = mp->mheap;		      /* Reset heap state */
    rstk = mp->mrstack; 	      /* Reset the return stack */

    /* To unwind the dictionary, we can't just reset the pointer,
    we must walk back through the chain and release all the name
    buffers attached to the items allocated after the mark was
    made. */

    while (dict != NULL && dict != dictprot && dict != mp->mdict) {
        free(dict->wname);	      /* Release name string for item */
        dict = dict->wnext;	      /* Link to previous item */
    }
}

#ifdef BREAK

/*  ATL_BREAK  --  Asynchronously interrupt execution.	Note that this
    function only sets a flag, broken, that causes
    exword() to halt after the current word.  Since
    this can be called at any time, it daren't touch the
    system state directly, as it may be in an unstable
    condition. */

void atl_break()
{
    broken = True;		      /* Set break request */
}
#endif /* BREAK */

#ifdef FILEIO
/*  ATL_LOAD  --  Load a file into the system.	*/
int atl_load(FILE *fp) {
    int es = ATL_SNORM;
    char s[134];
    atl_statemark mk;
    atl_int scomm = atl_comment;      /* Stack comment pending state */
    dictword **sip = ip;	      /* Stack instruction pointer */
    char *sinstr = instream;	      /* Stack input stream */
    int lineno = 0;		      /* Current line number */

    atl_errline = 0;		      /* Reset line number of error */
    atl_mark(&mk);
    ip = NULL;			      /* Fool atl_eval into interp state */

//    while ( atl_fgetsp(s, 132, fp) != NULL) {
    while ( atl_fgetsp((char *)s, 132, fp) != 0) {
        lineno++;
        if ((es = atl_eval((char *)s)) != ATL_SNORM) {
            atl_errline = lineno;     /* Save line number of error */
            atl_unwind(&mk);
            break;
        }
    }
    /* If there were no other errors, check for a runaway comment.  If
    we ended the file in comment-ignore mode, set the runaway comment
    error status and unwind the file.  */
    if ((es == ATL_SNORM) && (atl_comment == Truth)) {
#ifdef MEMMESSAGE

        char buffer[80];
        sprintf(buffer,"\nRunaway `(' comment.\n");
#ifdef MBED
        atlastTxString( buffer );
#endif
#endif
        es = ATL_RUNCOMM;
        atl_unwind(&mk);
    }
    atl_comment = scomm;	      /* Unstack comment pending status */
    ip = sip;			      /* Unstack instruction pointer */
    instream = sinstr;		      /* Unstack input stream */
    return es;
}
#endif
/*  ATL_PROLOGUE  --  Recognise and process prologue statement.
    Returns 1 if the statement was part of the
    prologue and 0 otherwise. */

int atl_prologue( char *sp) {
    static struct {
        char *pname;
        atl_int *pparam;
    } proname[] = {
        {"STACK ", &atl_stklen},
        {"RSTACK ", &atl_rstklen},
        {"HEAP ", &atl_heaplen},
        {"TEMPSTRL ", &atl_ltempstr},
        {"TEMPSTRN ", &atl_ntempstr}
    };

    if (strncmp(sp, "\\ *", 3) == 0) {
        int i;
        char *vp = sp + 3, *ap;

        ucase(vp);
        for (i = 0; i < (int)ELEMENTS(proname); i++) {
            if (strncmp(sp + 3, proname[i].pname,
                        strlen(proname[i].pname)) == 0) {
                if ((ap = strchr(sp + 3, ' ')) != NULL) {
                    V sscanf(ap + 1, "%li", proname[i].pparam);
#ifdef PROLOGUEDEBUG
                    char buffer[80];
                    sprintf(buffer,"Prologue set %sto %ld\n", proname[i].pname, *proname[i].pparam);
#ifdef MBED
                    atlastTxString(buffer);
#endif
#endif
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*  ATL_EVAL  --  Evaluate a string containing ATLAST words.  */

int atl_eval(char *sp) {
    int i;

#undef Memerrs
#define Memerrs evalstat
    instream = sp;
    evalstat = ATL_SNORM;	      /* Set normal evaluation status */
#ifdef BREAK
    broken = False;		      /* Reset asynchronous break */
#endif

    /* If automatic prologue processing is configured and we haven't yet
    initialised, check if this is a prologue statement.	If so, execute
    it.	Otherwise automatically initialise with the memory specifications
    currently operative. */

#ifdef PROLOGUE
    if (dict == NULL) {
        if (atl_prologue(sp))
            return evalstat;
        atl_init();
    }
#endif /* PROLOGUE */

    while ((evalstat == ATL_SNORM) && (i = token(&instream)) != TokNull) {
        dictword *di;

        switch (i) {
            case TokWord:
                if (forgetpend) {
                    forgetpend = False;
                    ucase(tokbuf);
                    if ((di = lookup(tokbuf)) != NULL) {
                        dictword *dw = dict;

                        /* Pass 1.  Rip through the dictionary to make sure
                        this word is not past the marker that
                        guards against forgetting too much. */

                        while (dw != NULL) {
                            if (dw == dictprot) {
#ifdef MEMMESSAGE
                                char buffer[40];
                                sprintf(buffer,"\nForget protected.\n");
#endif
                                evalstat = ATL_FORGETPROT;
                                di = NULL;
                            }
                            if (strcmp(dw->wname + 1, tokbuf) == 0)
                                break;
                            dw = dw->wnext;
                        }

                        /* Pass 2.  Walk back through the dictionary
                        items until we encounter the target
                        of the FORGET.  Release each item's
                        name buffer and dechain it from the
                        dictionary list. */

                        if (di != NULL) {
                            do {
                                dw = dict;
                                if (dw->wname != NULL)
                                    free(dw->wname);
                                dict = dw->wnext;
                            } while (dw != di);
                            /* Finally, back the heap allocation pointer
                            up to the start of the last item forgotten. */
                            hptr = (stackitem *) di;
                            /* Uhhhh, just one more thing.  If this word
                            was defined with DOES>, there's a link to
                            the method address hidden before its
                            wnext field.  See if it's a DOES> by testing
                            the wcode field for P_dodoes and, if so,
                            back up the heap one more item. */
                            if (di->wcode == (codeptr) P_dodoes) {
#ifdef FORGETDEBUG
                                char buffer[40];
                                sprintf(buffer," Forgetting DOES> word. ");
#ifdef MBED
                                atlastTxString( buffer );
#endif
#endif
                                hptr--;
                            }
                        }
                    } else {
#ifdef MEMMESSAGE
#ifdef EMBEDDED
                        char buffer[40];
                        sprintf(buffer," '%s' undefined ", tokbuf);
#ifdef MBED
                        atlastTxString( buffer );
#endif
#endif
#endif
                        // evalstat = ATL_UNDEFINED;
                    }
                } else if (tickpend) {
                    tickpend = False;
                    ucase(tokbuf);
                    if ((di = lookup(tokbuf)) != NULL) {
                        So(1);
                        Push = (stackitem) di; /* Push word compile address */
                    } else {
#ifdef MEMMESSAGE
                        char buffer[40];
                        sprintf(buffer," '%s' undefined ", tokbuf); // EMBEDDED
#ifdef MBED
                        atlastTxString( buffer );
#endif
#endif
                        evalstat = ATL_UNDEFINED;
                    }
                } else if (defpend) {
                    /* If a definition is pending, define the token and
                    leave the address of the new word item created for
                    it on the return stack. */
                    defpend = False;
                    ucase(tokbuf);
                    if (atl_redef && (lookup(tokbuf) != NULL)) {
                        char buffer[40];
                        sprintf(buffer,"\n%s isn't unique.", tokbuf);
#ifdef MBED
                        atlastTxString( buffer );
#endif
                    }

                    enter(tokbuf);
                } else {
                    di = lookup(tokbuf);
                    if (di != NULL) {
                        /* Test the state.  If we're interpreting, execute
                        the word in all cases.  If we're compiling,
                        compile the word unless it is a compiler word
                        flagged for immediate execution by the
                        presence of a space as the first character of
                        its name in the dictionary entry. */
                        if (state &&
                                (cbrackpend || ctickpend ||
                                !(di->wname[0] & IMMEDIATE))) {
                            if (ctickpend) {
                                /* If a compile-time tick preceded this
                                word, compile a (lit) word to cause its
                                address to be pushed at execution time. */
                                Ho(1);
                                Hstore = s_lit;
                                ctickpend = False;
                            }
                            cbrackpend = False;
                            Ho(1);	  /* Reserve stack space */
                            Hstore = (stackitem) di;/* Compile word address */
                        } else {
                            exword(di);   /* Execute word */
                        }
                    } else {
#ifdef MEMMESSAGE
                        char buffer[40];
                        sprintf(buffer," '%s' undefined\r\n", tokbuf);
#ifdef MBED
                        atlastTxString( buffer );
#endif
#endif
                        evalstat = ATL_UNDEFINED;
                        state = Falsity;
                    }
                }
                break;

            case TokInt:
                if (state) {
                    Ho(2);
                    Hstore = s_lit;   /* Push (lit) */
                    Hstore = tokint;  /* Compile actual literal */
                } else {
                    So(1);
                    Push = tokint;
                }
                break;

#ifdef REAL
            case TokReal:
                if (state) {
                    int i;
                    union {
                        atl_real r;
                        stackitem s[Realsize];
                    } tru;

                    Ho(Realsize + 1);
                    Hstore = s_flit;  /* Push (flit) */
                    tru.r = tokreal;
                    for (i = 0; i < Realsize; i++) {
                        Hstore = tru.s[i];
                    }
                } else {
                    int i;
                    union {
                        atl_real r;
                        stackitem s[Realsize];
                    } tru;

                    So(Realsize);
                    tru.r = tokreal;
                    for (i = 0; i < Realsize; i++) {
                        Push = tru.s[i];
                    }
                }
                break;
#endif /* REAL */

#ifdef STRING
            case TokString:
                if (stringlit) {
                    stringlit = False;
                    if (state) {
                        int l = (strlen(tokbuf) + 1 + sizeof(stackitem)) /
                            sizeof(stackitem);
                        Ho(l);
                        *((char *) hptr) = l;  /* Store in-line skip length */
                        V strcpy(((char *) hptr) + 1, tokbuf);
                        hptr += l;
                    } else {
                        char buffer[80];
                        sprintf(buffer,"%s", tokbuf);
#ifdef MBED
                        atlastTxString( buffer );
#endif
                    }
                } else {
                    if (state) {
                        int l = (strlen(tokbuf) + 1 + sizeof(stackitem)) /
                            sizeof(stackitem);
                        Ho(l + 1);
                        /* Compile string literal instruction, followed by
                        in-line skip length and the string literal */
                        Hstore = s_strlit;
                        *((char *) hptr) = l;  /* Store in-line skip length */
                        V strcpy(((char *) hptr) + 1, tokbuf);
                        hptr += l;
                    } else {
                        So(1);
                        V strcpy(strbuf[cstrbuf], tokbuf);
                        Push = (stackitem) strbuf[cstrbuf];
                        cstrbuf = (cstrbuf + 1) % ((int) atl_ntempstr);
                    }
                }
                break;
#endif /* STRING */
            default:
                {
                    char buffer[80];
                    sprintf(buffer,"\nUnknown token type %d\n", i);
#ifdef MBED
                    atlastTxString( buffer );
#endif
                }
                break;
        }
    }
    return evalstat;
}
