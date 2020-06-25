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

Queue<message_t, 8> tasks[(int)taskId::LAST];
MemoryPool<message_t, 8> mpool;
Serial *pc ;
Mutex stdio_mutex;

bool remoteCommand = true;
bool remoteProtocol() ;
/*
 * SDBlockDevice blockDevice(PA_7, PA_6, PA_5, PA_8);
 * LittleFileSystem fileSystem("fs");
 */

void initFs() {

    bool fail=true;
    atlastTxString((char *)"\r\nSetup filesystem\r\n");

    //    blockDevice = new SDBlockDevice (PA_7, PA_6, PA_5, PA_8);
    blockDevice = new SDBlockDevice (SPI_MOSI,  SPI_MISO, SPI_SCK,SPI_CS);
    fileSystem = new LittleFileSystem("fs");

    atlastTxString((char *)"Mounting the filesystem... \r\n");

    int err = fileSystem->mount(blockDevice);

    if(err) {
        atlastTxString((char *)"\r\nNo filesystem found, formatting...\r\n");
        err = fileSystem->reformat(blockDevice);

        if(err) {
            atlastTxString((char *)"... format failed.\r\n");
            bool fail=true;
        } else {
            atlastTxString((char *)"... done.\r\n");
            bool fail=false;
        }
    } else {
        atlastTxString((char *)"... done.\r\n");
        bool fail=false;
    }

    if(fail == false) {
        FILE *fd=fopen("/fs/numbers.txt", "r+");
        if(!fd) {
            atlastTxString((char *)" failed to open file.\r\n");
        } else {
            atlastTxString((char *)"closing file.\r\n");
            fclose(fd);
        }
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

uint8_t getChar(Serial *port) {
    do {
        ThisThread::yield();
    } while(!port->readable());

    uint8_t c=port->getc();

    return c;
}

uint8_t getCharEcho(Serial *port) {
    uint8_t c;
    port->putc( c=port->getc());

    return c;
}

int getBuffer(Serial *port, uint8_t *buffer, const uint8_t len) {

    uint8_t c=0;
    uint8_t cnt=0;

    for(int i=0 ; i< len;i++) {
        c = getChar(port);
        buffer[i] = c;
        cnt++;
    }
    return cnt;
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

    int iam = (int) taskId::ATLAST;

    sprintf((char *)lineBuffer,": IAM %d ;" ,iam);
    atl_eval((char *)lineBuffer);

    sprintf((char *)lineBuffer,": DBASE %llu ;",(long long unsigned int)db);
    atl_eval((char *)lineBuffer);

    sprintf((char *)lineBuffer,": MSG-PARSER %llu ;",(long long unsigned int) new parseMsg( db ));
    atl_eval((char *)lineBuffer);

    if(remoteCommand == false) {
        stdio_mutex.lock();
        ATH_banner();
        initFs();
        stdio_mutex.unlock();
    }

    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        (void)memset((char *)lineBuffer,0,sizeof(lineBuffer));

        if(remoteCommand == false) {
            sprintf(outBuffer, "\n\r-> ");
#ifdef MBED
            stdio_mutex.lock();
            atlastTxString(outBuffer);
            stdio_mutex.unlock();
#endif

            len = getline(pc, lineBuffer, MAX_LINE) ;
            atlastTxString((char *)"\r\n");
            atl_eval((char *)lineBuffer);
        } else {
            remoteCommand = remoteProtocol();
        }
    }
    //    return 0;
}

enum class remoteState {
    INVALID,
    START,
    CMD,
    DEST_LEN,
    DEST,
    LAST
};

bool remoteProtocol() {
    bool rc=false;
    static uint8_t c = 0xff;

    char tmpBuffer[32];

    static remoteState State;
    State =remoteState::INVALID;

    static highLevelOperation cmd;
    cmd = highLevelOperation::NOP;

    static uint8_t elementCount=0;
    static uint8_t destLen =0 ;
    static uint8_t destName[32];

    DigitalOut Led2(LED2);
    DigitalOut Led3(LED3);
    DigitalOut Led6(LED6);

    Led2=1;
    Led3=1;
    Led6=1;

    while(rc == false) {
        /*
         *        do {
         *            c = getChar(pc);
         } while ( c != '*');

         State =remoteState::START;
         */

        switch (State) {
            case remoteState::INVALID:
                c = getCharEcho(pc);

                /*
                 *                sprintf(tmpBuffer,(char *)"INVALID:0x%02x\r\n",c);
                 *                atlastTxString(tmpBuffer);
                 */
                if ( c == '*') {
                    State =remoteState::START;
                    Led2=1;
                    Led3=0;
                    Led6=0;
                } else {
                    State =remoteState::INVALID;
                }
                break;
            case remoteState::START:
                c = getCharEcho(pc);
                elementCount = c;

                if( elementCount >=1 and elementCount <= 4) {
                    State =remoteState::CMD;
                    Led2=0;
                    Led3=1;
                } else {
                    State =remoteState::INVALID;
                }
                break;
            case remoteState::CMD:
                {
                    uint8_t c1 = getCharEcho(pc);
                    cmd = (highLevelOperation)c1;

                    Led3=0;

                    switch(elementCount) {
                        case 3:
                            Led6=1;
                            switch(cmd) {
                                case highLevelOperation::SET:
                                    Led2=1;
                                    State = remoteState::DEST_LEN;
                                    break;
                                case highLevelOperation::SUB:
                                    State = remoteState::DEST_LEN;
                                    break;
                                case highLevelOperation::UNSUB:
                                    State = remoteState::DEST_LEN;
                                    break;
                                default:
                                    State =remoteState::INVALID;
                                    break;
                            }
                            break;
                        default:
                            State =remoteState::INVALID;
                            break;
                    }
                }
                break;
            case remoteState::DEST_LEN:
                {
                    uint8_t dLen = getCharEcho(pc);

                    if (dLen > 32 ) {
                        State =remoteState::INVALID;
                    } else {
                        State =remoteState::DEST;
                        destLen = (uint8_t) dLen;
                        Led2=1;
                        Led3=1;
                        Led6=1;
                    }
                }
                break;
            case remoteState::DEST:
            {
                uint8_t d=0;
                uint8_t buffer[32];
                uint8_t i;
                for(i=0;i< destLen;i++) {
                    d = getCharEcho(pc);
                    buffer[i] = d;
                }
                memcpy(destName,buffer,i);
            }
                break;
            default:
                break;
        }
    }

    return rc;
}

void atlastRx(Small *db) {
    int iam = (int) taskId::ATLAST;
    parseMsg *p = new parseMsg( db );

    while(true) {
        osEvent evt = tasks[iam].get();

        if (evt.status == osEventMessage ) {
            message_t *message = (message_t*)evt.value.p;

            /*
             *            stdio_mutex.lock();
             *            atlastTxString((char *)"\n=========");
             *            printIam((taskId)iam);
             *            msgDump(message);
             *            atlastTxString((char *)"=========\n");
             *            stdio_mutex.unlock();
             */

            bool fail = p->fromMsgToDb(message);

            mpool.free(message);
        }
    }
}

int main() {
    pc = new Serial(SERIAL_TX, SERIAL_RX);

    pc->baud(115200);

    //    initFs();

    osStatus status ;
    Small *atlastDb = new Small();

    Thread i2cThread;
    extern void i2cTask();

    status = i2cThread.start(callback(i2cTask));

    if(remoteCommand == false) {
        stdio_mutex.lock();
        if (status == osOK) {
            atlastTxString((char *)"i2cThread started\r\n");
        } else {
            atlastTxString((char *)"i2cThread failed\r\n");
        }
        stdio_mutex.unlock();
    }

    Thread ledThread;
    status = ledThread.start(ledControlTask);

    if(remoteCommand == false) {
        stdio_mutex.lock();
        if (status == osOK) {
            atlastTxString((char *)"ledThread started\r\n");
        } else {
            atlastTxString((char *)"ledThread failed\r\n");
        }
        stdio_mutex.unlock();
    }

    Thread atlastRxThread;
    status = atlastRxThread.start(callback(atlastRx,atlastDb));

    if(remoteCommand == false) {
        stdio_mutex.lock();
        if (status == osOK) {
            atlastTxString((char *)"atlastRxThread started\r\n");
        } else {
            atlastTxString((char *)"atlastRxThread failed\r\n");
        }
        stdio_mutex.unlock();
    }

    Thread atlastThread;
    status = atlastThread.start(callback(atlast,atlastDb));

    if(remoteCommand == false) {
        stdio_mutex.lock();
        if (status == osOK) {
            atlastTxString((char *)"atlastThread started\r\n");
        } else {
            atlastTxString((char *)"atlastThread failed\r\n");
        }
        stdio_mutex.unlock();

    }
    atlastRxThread.join();
    atlastThread.join();
    ledThread.join();
}


