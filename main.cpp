#include "mbed.h"
#include "msg.h"
#include "tasks.h"
// #include "Small.h"
#include "mbedSmall.h"

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
    #include "parseMsg.h"
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
 
    DigitalOut myLed(LED1);    
    
    Small *db = new mbedSmall();
    parseMsg *p = new parseMsg( db );
    
    db->Set("LED1","ON");
    
    uint32_t dly = 250;
   
    bool fail=true;
    
    while(runFlag) {    
        osEvent evt = tasks[iam].get( dly );    
        
        if (evt.status == osEventMessage ) {    
            message_t *message = (message_t*)evt.value.p;
            char *k;
            taskId id;
            
            switch(message->op.hl_op) {
                case highLevelOperation::NOP :
                    break;
                case highLevelOperation::GET :
                    break;
                case highLevelOperation::SET :
                    fail = p->fromMsgToDb(message);
                    break;
                case highLevelOperation::SUB :
                    id = p->getSender(message);
                    k = p->getKey(message);
                    
                    db->Sub(k,(uint8_t)id);
                    break;
            }
            /*
            if (message->op.hl_op == highLevelOperation::SET) {
            }
            */
            
            mpool.free(message);    
        } 
        
        string ledState = db->Get("LED1");
        if (ledState == "ON") {
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
    // char t[132];
    int8_t len;
    //    Serial pc(USBTX, USBRX);
    
    pc = new Serial(USBTX, USBRX);
    
    bool runFlag=true;
    
    uint8_t lineBuffer[MAX_LINE];
    // dictword *var;
    //    int *tst;
    
    pc->baud(115200);
    
    atl_init();
    
    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval((char *)lineBuffer);
    } while(len >= 0);
    
    cpp_extrasLoad();
    
    /*
     *    var = atl_vardef((char *)"DBASE",sizeof(Small *));
     * 
     *    if(var == NULL) {
     *        fprintf(stderr,"Vardef failed\n");
} else {
    //        *((int *)atl_body(var))=42;
    *((stackitem *)atl_body(var))=(stackitem)db;
}
*/
    
    int iam = (int) taskId::ATLAST;
    
    sprintf((char *)lineBuffer,": IAM %d ;" ,iam);
    atl_eval((char *)lineBuffer);
    
    sprintf((char *)lineBuffer,": DBASE %llu ;",(long long unsigned int)db);
    atl_eval((char *)lineBuffer);
    
    sprintf((char *)lineBuffer,": MSG-PARSER %llu ;",(long long unsigned int) new parseMsg( db ));
    atl_eval((char *)lineBuffer);
    
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
    parseMsg *p = new parseMsg( db );
    
    while(true) {
        osEvent evt = tasks[iam].get(  );
        
        if (evt.status == osEventMessage ) {
            message_t *message = (message_t*)evt.value.p;
            
            bool fail = p->fromMsgToDb(message);
            
            /*
            taskId From = message->Sender;
            msgType type = message->type;
            char *topic = message->body.hl_body.topic;
            char *msg = message->body.hl_body.msg;
            */
            
            mpool.free(message);
        }
        
//        ThisThread::yield();
    }
}

int main() {
    Small *atlastDb = new Small();
    
    Thread ledThread;
    ledThread.start(ledControlTask);
    
    Thread atlastRxThread;
    atlastRxThread.start(callback(atlastRx,atlastDb));
    
    Thread atlastThread;
    atlastThread.start(callback(atlast,atlastDb));
    
    atlastThread.join();
    
    ledThread.join();
}


