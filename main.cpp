#include "mbed.h"
#include "msg.h"
#include "tasks.h"
#include "Small.h"
#define ECHO
extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "atlast.h"
#include "atlcfig.h"
#include "atldef.h"

#include "extraFunc.h"
#include "io.h"
#ifdef NVRAMRC
#include "nvramrc.h"
#else
    uint8_t nvramrc[] = ": tst \n10 0 do \ni . cr \nloop \n; \n \n";
    // uint8_t nvramrc[] = ": sifting\ntoken $sift\n;\n: [char] char ;";
#endif

    char outBuffer[OUTBUFFER];
}

Queue<message_t, 8> tasks[(int)taskId::LAST];    
MemoryPool<message_t, 8> mpool;    
Serial *pc ;


void ledControlTask(void) {    
    
    int iam = (int) taskId::LED_CTRL;    
//    Queue<message_t, 8> myQueue = tasks[iam];    
    
    bool runFlag = true;    
    bool blink = true;
    DigitalOut myLed(LED1);    

    uint32_t dly = 250;
//    uint32_t dly = osWaitForever;
    
    while(runFlag) {    
        osEvent evt = tasks[iam].get( dly );    
    
        if (evt.status == osEventMessage ) {    
            message_t *message = (message_t*)evt.value.p;    
    
            char *topic = message->body.hl_body.topic;    
    
            if(!strcmp(topic,"LED1")) {    
                char *msg = message->body.hl_body.msg;    
                if(!strcmp(msg,"ON")) {    
                    blink = true;
                    dly = 250;
//                    myLed=1;    
                } else if(!strcmp(msg,"OFF")) {    
                    blink = false;
                    dly = osWaitForever;
//                    myLed=0;    
                }    
            }    
    
            mpool.free(message);    
        } else {
            if (blink) {
                if(myLed == 1) {
                    myLed = 0;
                } else {
                    myLed = 1;
                }
            } else {
                myLed = 0;
            }
        }
    }    
}

int getline(Serial *port, uint8_t *b, const uint8_t len) {

    bool done=false;
    uint8_t count=0;
    char c;

    while ( done == false ) {
        c=port->getc();
#ifdef ECHO
        port->putc(c);
#endif

        if(iscntrl(c)) {
            switch(c) {
                case '\n':
                case '\r':
                    b[count] = '\0';
                    done=true;
                    break;
                case 0x08:
                    //                        port->putc(0x08);
                    port->putc(' ');
                    port->putc(0x08);
                    count--;
                    b[count] = '\0';
                    break;
            }
        } else {
            b[count++] = c;
        }
    }
    return(count);
}

void  atlast(Small *db) {
    char t[132];
    int8_t len;
    //    Serial pc(USBTX, USBRX);

    pc = new Serial(USBTX, USBRX);

    bool runFlag=true;

    uint8_t lineBuffer[MAX_LINE];
    dictword *var;
    //    int *tst;

    pc->baud(115200);

    atl_init();

    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval((char *)lineBuffer);
    } while(len >= 0);

    cpp_extrasLoad();

    var = atl_vardef((char *)"DBASE",sizeof(Small *));

    if(var == NULL) {
        fprintf(stderr,"Vardef failed\n");
    } else {
//        *((int *)atl_body(var))=42;
        *((stackitem *)atl_body(var))=(stackitem)db;
    }

    ATH_banner();

    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        (void)memset((char *)lineBuffer,0,sizeof(lineBuffer));

        sprintf(outBuffer, "\n\r-> ");
#ifdef MBED
        atlastTxString(outBuffer);
#endif
//        pc->printf("\n\r-> ");

        len = getline(pc, lineBuffer, MAX_LINE) ;
        atl_eval((char *)lineBuffer);
    }
//    return 0;
}

void atlastRx(Small *db) {
    int iam = (int) taskId::ATLAST;

    while(true) {
        osEvent evt = tasks[iam].get(  );

        if (evt.status == osEventMessage ) {
            message_t *message = (message_t*)evt.value.p;
    
            taskId From = message->Sender;
            msgType type = message->type;
            char *topic = message->body.hl_body.topic;
            char *msg = message->body.hl_body.msg;
        }

        ThisThread::sleep_for(0);
    }
}

int main() {
    Small *atlastDb = new Small();

    Thread ledThread;
    ledThread.start(ledControlTask);

    Thread atlastRxThread;
    atlastRxThread.start(atlastRx,atlastDb);

    Thread atlastThread;
    atlastThread.start(atlast,atlastDb);

    atlastThread.join();

    ledThread.join();
}


