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
    INVALID=0,
    START,
    CMD,
    DEST_LEN,
    DEST,
    KEY_LEN,
    KEY,
    VALUE_LEN,
    VALUE,
    END,
    LAST
};

bool remoteProtocol() {
    bool rc=false;
    static uint8_t c = 0xff;

    char tmpBuffer[32];

    static remoteState State;
    State =remoteState::INVALID;

    volatile highLevelOperation cmd;
    cmd = highLevelOperation::NOP;

    static uint8_t elementCount=0;
    static uint8_t destLen =0 ;
    static uint8_t destName[32];

    static uint8_t keyLen =0 ;
    static uint8_t keyName[32];

    static uint8_t valueLen =0 ;
    static uint8_t value[32];


    /*
    DigitalOut Led2(LED2);
    DigitalOut Led3(LED3);
    DigitalOut Led6(LED6);

    Led2=1;
    Led3=1;
    Led6=1;
    */

    I2C *i2c = new I2C(I2C_SDA, I2C_SCL);

    char i2cCmd[2] ;

    i2cCmd[0] = i2cCmd[1] = 0;

    i2c->write(0x40,i2cCmd,1);

//     while(rc == false) {
    while(remoteCommand == true) {

        switch (State) {
            case remoteState::INVALID:
                c = getCharEcho(pc);

                if ( c == '*') {
                    State =remoteState::START;
                } else {
                    State =remoteState::INVALID;
                }
                break;

            case remoteState::START:
                i2cCmd[0] = (char)remoteState::START;
                i2c->write(0x40,i2cCmd,1);

                c = getCharEcho(pc);
                elementCount = c;

                if( elementCount >=1 and elementCount <= 4) {
                    State =remoteState::CMD;
                } else {
                    State =remoteState::INVALID;
                }
                break;
            case remoteState::CMD:
                i2cCmd[0] = (char)remoteState::CMD;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t c1 = getCharEcho(pc);
                    cmd = (highLevelOperation)c1;

                    switch(elementCount) {
                        case 1:
                            if(cmd == highLevelOperation::EXIT) {
                                State = remoteState::END;
                                remoteCommand = false;

                            } else if(cmd == highLevelOperation::NOP) {
                                State = remoteState::END;
                            }
                            break;
                        case 3:
                            switch(cmd) {
                                case highLevelOperation::GET:
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
                        case 4:
                            if(cmd == highLevelOperation::SET) {
                                    State = remoteState::DEST_LEN;
                            } else {
                                    State =remoteState::INVALID;
                            }
                            break;
                        default:
                            State =remoteState::INVALID;
                            break;
                    }
                }
                break;
            case remoteState::DEST_LEN:
                i2cCmd[0] = (char)remoteState::DEST_LEN;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t dLen = getCharEcho(pc);

                    if (dLen > 32 ) {
                        State =remoteState::INVALID;
                    } else {
                        State =remoteState::DEST;
                        destLen = (uint8_t) dLen;
                    }
                }
                break;
            case remoteState::DEST:
                i2cCmd[0] = (char)remoteState::DEST;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t d=0;
                    uint8_t buffer[32];
                    uint8_t i;
                    for(i=0;i< destLen;i++) {
                        d = getCharEcho(pc);
                        buffer[i] = d;
                    }
                    memcpy(destName,buffer,i);
                    State=remoteState::KEY_LEN;
                }
                break;
            case remoteState::KEY_LEN:
                i2cCmd[0] = (char)remoteState::KEY_LEN;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t kLen = getCharEcho(pc);
                    keyLen = kLen;
                }
                State=remoteState::KEY;
                break;
            case remoteState::KEY:
                i2cCmd[0] = (char)remoteState::KEY;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t buffer[32];
                    uint8_t i;
                    for(i=0;i< keyLen;i++) {
                        uint8_t d;
                        d = getCharEcho(pc);
                        buffer[i] = d;
                    }
                    memcpy(keyName,buffer,i);
                }
                State=remoteState::VALUE_LEN;
                break;
            case remoteState::VALUE_LEN:
                i2cCmd[0] = (char)remoteState::VALUE_LEN;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t vLen = getCharEcho(pc);
                    valueLen = vLen;
                }
                State=remoteState::VALUE;
                break;
            case remoteState::VALUE :
                i2cCmd[0] = (char)remoteState::VALUE;
                i2c->write(0x40,i2cCmd,1);
                {
                    uint8_t buffer[32];
                    uint8_t i;
                    for(i=0;i< valueLen;i++) {
                        uint8_t d;
                        d = getCharEcho(pc);
                        buffer[i] = d;
                    }
                    memcpy(value,buffer,i);
                }
                State=remoteState::END;
                break;
            case remoteState::END:
                i2cCmd[0] = (char)0xff;
                i2c->write(0x40,i2cCmd,1);
                State=remoteState::INVALID;
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


