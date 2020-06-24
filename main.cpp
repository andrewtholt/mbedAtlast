#include "mbed.h"

#include <errno.h>

#include "msg.h"
#include "tasks.h"
// #include "Small.h"
#include "mbedSmall.h"
#include "atlastUtils.h"


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

#include "SDBlockDevice.h"
SDBlockDevice *blockDevice;

#include "LittleFileSystem.h"
LittleFileSystem *fileSystem;

extern void initFs();

Queue<message_t, 8> tasks[(int)taskId::LAST];
MemoryPool<message_t, 8> mpool;
Serial *pc ;
Mutex stdio_mutex;

/*
SDBlockDevice blockDevice(PA_7, PA_6, PA_5, PA_8);
LittleFileSystem fileSystem("fs");
*/

void initFs() {
    atlastTxString((char *)"\r\nSetup filesystem\r\n");
    blockDevice = new SDBlockDevice (PA_7, PA_6, PA_5, PA_8);
    fileSystem = new LittleFileSystem("fs");

    atlastTxString((char *)"Mounting the filesystem... \r\n");

    int err = fileSystem->mount(blockDevice);

    if(err) {
        atlastTxString((char *)"\r\nNo filesystem found, formatting...\r\n");
        err = fileSystem->reformat(blockDevice);

        if(err) {
            error("error: %s (%d)\r\n", strerror(-err), err);
        } else {
            atlastTxString((char *)"... done.\r\n");
        }
    } else {
        atlastTxString((char *)"... done.\r\n");
    }

    FILE *fd=fopen("/fs/numbers.txt", "r+");
    if(!fd) {
        atlastTxString((char *)" failed to open file.\r\n");
    } else {
        atlastTxString((char *)"closing file.\r\n");
        fclose(fd);
    }
}

void ledControlTask(void) {

    int count=-1;
    taskId iam = taskId::LED_CTRL;

    bool runFlag = true;


//    DigitalOut myLed(LED1);
    DigitalOut myLed(PD_14);

    mbedSmall *db = new mbedSmall();
    parseMsg *p = new parseMsg( db );

    db->Set("LED1","ON");

    uint32_t dly = 250;

    bool fail=true;

    while(runFlag) {
        osEvent evt = tasks[(int)iam].get( dly );

        count++;

        char buffer[32];
        sprintf(buffer,"%08d",count);

        db->Set("COUNT", buffer);

        if (evt.status == osEventMessage ) {
            message_t *message = (message_t*)evt.value.p;

            stdio_mutex.lock();

            atlastTxString((char *)"\n=========");
            printIam((taskId)iam);
            msgDump(message);
            atlastTxString((char *)"=========\n");

            stdio_mutex.unlock();

            char *k;
            taskId id;

            switch(message->op.hl_op) {
                case highLevelOperation::NOP :
                    break;
                case highLevelOperation::GET :
                    k = p->getKey(message);
                    {
                        taskId to = p->getSender(message) ;

                        std::string v=db->Get( k );
                        db->sendSet(iam, to,k, v.c_str()) ;
                    }
                    break;
                case highLevelOperation::SET :
                    fail = p->fromMsgToDb(message);
                    break;
                case highLevelOperation::SUB :
                    id = p->getSender(message);
                    k = p->getKey(message);

                    db->Sub(k,(uint8_t)id);
                    break;
                case highLevelOperation::UNSUB :
                    break;
            }

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

        do {
            ThisThread::yield();
        } while(!port->readable());

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
    int8_t len;
    bool runFlag=true;
    uint8_t lineBuffer[MAX_LINE];

    atl_init();

    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval((char *)lineBuffer);
    } while(len >= 0);

    extern void cpp_extrasLoad();
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

        stdio_mutex.lock();
    ATH_banner();
//    initFs();
        stdio_mutex.unlock();

    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        (void)memset((char *)lineBuffer,0,sizeof(lineBuffer));

        sprintf(outBuffer, "\n\r-> ");
        #ifdef MBED
        stdio_mutex.lock();
        atlastTxString(outBuffer);
        stdio_mutex.unlock();
        #endif

        len = getline(pc, lineBuffer, MAX_LINE) ;
        atl_eval((char *)lineBuffer);
    }
    //    return 0;
}

void atlastRx(Small *db) {
    int iam = (int) taskId::ATLAST;
    parseMsg *p = new parseMsg( db );

    while(true) {
        osEvent evt = tasks[iam].get();

        if (evt.status == osEventMessage ) {
            message_t *message = (message_t*)evt.value.p;

            /*
            stdio_mutex.lock();
            atlastTxString((char *)"\n=========");
            printIam((taskId)iam);
            msgDump(message);
            atlastTxString((char *)"=========\n");
            stdio_mutex.unlock();
            */

            bool fail = p->fromMsgToDb(message);

            mpool.free(message);
        }
    }
}

int main() {
//    extern void initFs();
    pc = new Serial(PA_2, PA_3);

//    pc = new Serial(USBTX, USBRX);
    pc->baud(115200);
    atlastTxString((char *)"\r\nHello\r\n");

    /*
    atlastTxString((char *)"\r\nSetup filesystem\r\n");
    atlastTxString((char *)"Mounting the filesystem... \r\n");

    int err = fileSystem.mount(&blockDevice);

    if(err) {
        atlastTxString((char *)"\r\nNo filesystem found, formatting...\r\n");
        err = fileSystem.reformat(&blockDevice);

        if(err) {
            error("error: %s (%d)\r\n", strerror(-err), err);
        } else {
            atlastTxString((char *)"... done.\r\n");
        }
    } else {
        atlastTxString((char *)"... done.\r\n");
    }

    FILE *fd=fopen("/fs/numbers.txt", "r+");
    if(!fd) {
        atlastTxString((char *)" failed to open file.\r\n");
    }
    */

//    initFs();

    osStatus status ;
    Small *atlastDb = new Small();

    Thread i2cThread;
    extern void i2cTask();

    status = i2cThread.start(callback(i2cTask));

    stdio_mutex.lock();
    if (status == osOK) {
        atlastTxString((char *)"i2cThread started\r\n");
    } else {
        atlastTxString((char *)"i2cThread failed\r\n");
    }
    stdio_mutex.unlock();

    Thread ledThread;
    status = ledThread.start(ledControlTask);
    stdio_mutex.lock();
    if (status == osOK) {
        atlastTxString((char *)"ledThread started\r\n");
    } else {
        atlastTxString((char *)"ledThread failed\r\n");
    }
    stdio_mutex.unlock();

    Thread atlastRxThread;
    status = atlastRxThread.start(callback(atlastRx,atlastDb));
    stdio_mutex.lock();
    if (status == osOK) {
        atlastTxString((char *)"atlastRxThread started\r\n");
    } else {
        atlastTxString((char *)"atlastRxThread failed\r\n");
    }
    stdio_mutex.unlock();

    Thread atlastThread;
    status = atlastThread.start(callback(atlast,atlastDb));

    stdio_mutex.lock();
    if (status == osOK) {
        atlastTxString((char *)"atlastRxThread started\r\n");
    } else {
        atlastTxString((char *)"atlastRxThread failed\r\n");
    }
    stdio_mutex.unlock();

    atlastRxThread.join();
    atlastThread.join();
    ledThread.join();
}


