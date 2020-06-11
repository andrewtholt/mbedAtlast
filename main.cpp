#include "mbed.h"
#include "msg.h"
#include "tasks.h"
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

Thread ledThread;

void ledControlTask(void) {    
    
    int iam = (int) taskId::LED_CTRL;    
//    Queue<message_t, 8> myQueue = tasks[iam];    
    
    bool runFlag = true;    
    DigitalOut myLed(LED1);    
    
    while(runFlag) {    
        osEvent evt = tasks[iam].get(osWaitForever);    
    
        if (evt.status == osEventMessage ) {    
            message_t *message = (message_t*)evt.value.p;    
    
            char *topic = message->body.hl_body.topic;    
    
            if(!strcmp(topic,"LED1")) {    
                char *msg = message->body.hl_body.msg;    
                if(!strcmp(msg,"ON")) {    
                    myLed=1;    
                } else if(!strcmp(msg,"OFF")) {    
                    myLed=0;    
                }    
            }    
    
            mpool.free(message);    
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

int main() {
    char t[132];
    int8_t len;
    //    Serial pc(USBTX, USBRX);

    Serial *pc = new Serial(USBTX, USBRX);

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

    var = atl_vardef((char *)"FOUR",4);

    /*
       if(var == NULL) {
       fprintf(stderr,"Vardef failed\n");
       } else {
     *((int *)atl_body(var))=42;
     }
     */

    cpp_extrasLoad();

    ledThread.start(ledControlTask);

    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        (void)memset((char *)lineBuffer,0,sizeof(lineBuffer));
        pc->printf("\n\r-> ");

        len = getline(pc, lineBuffer, MAX_LINE) ;
        atl_eval((char *)lineBuffer);
    }
    return 0;
}


