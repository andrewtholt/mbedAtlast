
#include "mbed.h"
#include "msg.h"
#include "tasks.h"
#include "SDBlockDevice.h"
#include "LittleFileSystem.h"
#include <stdio.h>
#include <errno.h>

#include "TM1637/TM1637.h"
TM1637_EYEWINK EYEWINK(D8, D9);

/*
SDBlockDevice blockDevice(PA_7, PA_6, PA_5, PA_8);
LittleFileSystem fileSystem("fs");
*/
extern SDBlockDevice *blockDevice;
extern LittleFileSystem *fileSystem;

// #include "Small.h"
#include "mbedSmall.h"
#include "parseMsg.h"

#include "io.h"
#include "tasks.h"
#include "atlastUtils.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool;
extern Serial *pc;
extern Mutex stdio_mutex;

char dataBuffer[255];  //Somewhat to put data.


extern "C" {
    #include "extraFunc.h"
    #include "atlast.h"
    #include "protocol.h"
    #include "atldef.h"

    extern int Fred(int);
}
extern bool remoteCommand;

using namespace std;

prim crap() {
}

prim P_listDirectory() {
//    DIR *d = opendir("/fs/");
    DIR *d = opendir(FS);

    if(!d) {
        atlastTxString((char *)"opendir failed\r\n");
    } else {
        atlastTxString((char *)"root directory:\n");
    while (true) {
        struct dirent *e = readdir(d);
        if (!e) {
            break;
        }
        atlastTxString((char*)"\r    ");
        atlastTxString(e->d_name);
        atlastTxString((char *) "\r\n");
        }
    }
}

// Stack: filename -- fail_flag
//
prim P_dCat() {
    bool fail = true;

    char *fname = (char *)S0;
    char buffer[255];
    char nameBuffer[255];

    strcat(nameBuffer,FS);
    strcat(nameBuffer, fname);

//    FILE *f = fopen(fname,"r") ;
    FILE *f = fopen(nameBuffer,"r") ;
    if(!f) {
        fail = true;
    } else {
        while (!feof(f)) {
            bzero(buffer,255);
            size_t ret= fread(buffer, 1,255, f);
            /*
            int c = fgetc(f);
            atlastTxByte(c);
            */
            atlastTxBuffer(buffer, ret);
        }
        fail = false;
    }
    S0 = (stackitem) fail;
}

prim P_dRm() {
    char nameBuffer[255];

    char *fname = (char *)S0;

    strcat(nameBuffer,FS);
    strcat(nameBuffer, fname);

//    S0 = (stackitem) (remove( fname) < 0) ? -1 : 0 ;
    S0 = (stackitem) (remove( nameBuffer) < 0) ? -1 : 0 ;
}

prim P_dTouch() {
    char nameBuffer[255];
    char *fname = (char *)S0;

    strcat(nameBuffer,FS);
    strcpy(nameBuffer, fname);

//    FILE *fd = fopen(fname, "a");
    FILE *fd = fopen(nameBuffer, "a");

    if(fd != 0) {
        fclose(fd);
    }
    Pop;
}

prim P_catFile() {
   ATH_Token();
   P_dCat();
}

prim P_rm() {
   ATH_Token();
   P_dRm();
}

prim P_touch() {
extern void ATH_Token();
   ATH_Token();
   P_dTouch();
}

extern uint8_t getChar(Serial *);
extern uint8_t getBuffer(Serial *, uint8_t *buffer, const uint8_t len);

prim P_DL() {
    char fname[255];
    char in[255];


    bzero(fname, sizeof(fname));
    bzero(in, sizeof(in));

    I2C *i2c = new I2C(I2C_SDA, I2C_SCL);
    char i2cCmd[2] ;

    i2cCmd[0] = i2cCmd[1] = 0;
    i2c->write(0x40,i2cCmd,1);

    uint8_t c;
    do {
        c = getChar(pc);

        if( c == CAN ) {
            return;
        }
    } while ( c != STX);

    i2cCmd[0] = c;
    i2c->write(0x40,i2cCmd,1);

    atlastTxByte(ACK);
    // Get length of filename
    uint8_t fnameLen = getChar(pc);

    // get Filename
    getBuffer(pc,(uint8_t *)in,fnameLen);

//    strcpy(fname,"/fs/");
    strcpy(fname,FS);
    strcat(fname,in);

    FILE *fd = fopen( fname, "w");

    // Failed to open file.
    //
    if(!fd) {
        atlastTxByte(NAK);
        return;
    } else {
        atlastTxByte(ACK);
    }
    uint8_t blockLen=0;
    uint8_t rdLen=0;

    int wrLen=0;
    do {
        bzero(in, sizeof(in));
        c = (uint8_t)getChar(pc);

        if( c == STX) {
    // Get block size (<= 255)
            blockLen = (uint8_t)getChar(pc);
    // Get block.
            rdLen = (uint8_t)getBuffer(pc,(uint8_t *)in,blockLen);
            wrLen=fwrite(in,1,rdLen,fd);

        i2cCmd[0] = wrLen;
        i2c->write(0x40,i2cCmd,1);

            fflush(fd);
            atlastTxByte(ACK);
        } else {
            break;
        }
    }
    while(true) ;
    atlastTxByte(ACK);
    fclose(fd);
}

prim P_setRemoteCommand() {
    remoteCommand = (bool)S0;
    Pop;
}

prim taskQdepth() {
    taskId tid = (taskId)S0;

    S0 = (stackitem)tasks[(int)tid].count();
}
//
// stack: <task-id> <msg> --
//
prim sendMsg() {

    int dest = (int) S1;
    message_t *out = (message_t *)S0;

    tasks[dest].put(out);

    Pop2;
}

prim getLowLevelType() {
    Push = (int) msgType::LO_LEVEL;
}

prim getHighLevelType() {
    Push = (int) msgType::HI_LEVEL;
}

prim opNop() {
    Push = (int) highLevelOperation::NOP;
}

prim opGet() {
    Push = (int) highLevelOperation::GET;
}

prim opSet() {
    Push = (int) highLevelOperation::SET;
}

prim opSub() {
    Push = (int) highLevelOperation::SUB;
}

prim opUnsub() {
    Push = (int) highLevelOperation::UNSUB;
}

prim getMainId() {
    Push = (int) taskId::ATLAST;
}

prim getLEDId() {
    Push = (int) taskId::LED_CTL;
}

prim getI2CId() {
    Push = (int) taskId::I2C;
}

prim mkMessage() {
    message_t *msg = mpool.calloc();
    Push = (stackitem) msg;
}
//
// Stack : Sender msg --
//
prim setSender() {
    message_t *msg = (message_t *)S0;

    msg->Sender = (taskId)S1;

    Pop2;
}
//
// Stack : type msg --
//
prim setType() {
    message_t *msg = (message_t *)S0;

    msg->type = (msgType)S1;

    Pop2;
}
//
// Stack : type msg --
//
prim setOp() {
    message_t *msg = (message_t *)S0;

    msg->op.hl_op = (highLevelOperation)S1;

    Pop2;
}

prim setTopic() {
    message_t *msg = (message_t *)S0;
    char *topic = (char *)S1;

    strcpy(msg->body.hl_body.topic,topic);

    Pop2;
}

prim setMsg() {
    message_t *msg = (message_t *)S0;

    char *payload = (char *)S1;

    strcpy(msg->body.hl_body.msg,payload);

    Pop2;
}

//
// Stack: db key value --
//
prim MBED_addRecord() {
//    Sl(3);

    char *value =(char *)S0;
    char *key = (char *)S1;
    Small *db = (Small *)S2;

    db->Set(key, value);
    Pop2;
    Pop;
//    So(0);
}
//
// Stack: db key --
prim MBED_lookup() {
    char *key = (char *)S0;
    Small *db = (Small *)S1;

    string tmp = db->Get(key);
    strcpy(dataBuffer, tmp.c_str());
    Pop;
    S0=(stackitem)dataBuffer;
}
//
// Find a record in the db and populate a message.
//
prim MBED_dbToMsg() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    bool fail = parse->fromDbToMsg( msg, sender, key);
    if(fail) {
        mpool.free(msg);
    }

    Npop(3);
    S0 = (stackitem)fail;
}

prim MBED_subscribe() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    mkSubMsg( msg, sender, key);

    Npop(4);
}

prim MBED_get() {

    extern void mkGetMsg(message_t*, taskId, char*);
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    mkGetMsg( msg, sender, key);
    msg->op.hl_op = highLevelOperation::GET;

    Npop(4);
}

prim MBED_msgDump() {
    message_t *msg = (message_t *)S0;

    stdio_mutex.lock();
    msgDump( msg );
    stdio_mutex.unlock();

    Pop;
}

prim subscribers() {
}

void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

/* Open file: fname fmodes fd -- flag */
prim P_fopen()	{
    stackitem stat;

//    Sl(3);
//    Hpc(S2);
//    Hpc(S0);

//    Isfile(S0);

    char nameBuffer[255];
    char * fname = (char *) S2;
    char *mode =(char *)S1;

    strcpy(nameBuffer, FS);
    strcat(nameBuffer,fname);

//    FILE *fd = fopen(fname,mode);
    FILE *fd = fopen(nameBuffer,mode);

    if (fd == NULL) {
        stat = false;
    } else {
        *(((stackitem *) S0) + 1) = (stackitem) fd;
        stat = true;
    }

    Pop2;
    S0 = stat;
}

/* Was ------- File read: fd len buf -- length */
/* ATH Now --- File read: buf len fd -- length */
prim P_fread() {
    /* Was ------- File read: fd len buf -- length */
    /* ATH Now --- File read: buf len fd -- length */
//    Sl(3);
//    Hpc(S0);

//     Isfile(S0);
//    Isopen(S0);
    // TODO This is stupid, it is inconsisitent with write.
    // The stack order should follow the C convention.
    // S2 = fread((char *) S0, 1, ((int) S1), FileD(S2));
//    S2 = fread((char *) S2, 1, ((int) S1), FileD(S0));

    FILE *fd = FileD(S0);
    int len = (int)S1;
    void *buffer = (void *)S2;

//    size_t ret= fread(buffer, len,1, fd);
    size_t ret= fread(buffer, 1,len, fd);

    Pop2;

    S0 = (stackitem) ret;
}

/* Close file: fd -- */
prim P_fclose() {
//    Sl(1);
//    Hpc(S0);

//    Isfile(S0);
//    Isopen(S0);
    V fclose(FileD(S0));

    *(((stackitem *) S0) + 1) = (stackitem) NULL;
    Pop;
}

// was : File write: len buf fd -- length
// Is : write: buff, size, fd -- length
prim P_fwrite() {
//    Sl(3);
//    Hpc(S2);
//    Isfile(S0);

//    Isopen(S0);

    FILE *fd = FileD(S0);
    int len = (int)S1;
    void *buffer = (void *)S2;

    size_t ret= fwrite(buffer, 1,len, fd);
//    S2 = fwrite((char *) S1, 1, ((int) S2), FileD(S0));
    Pop2;

    S0 = (stackitem) ret;
}

prim P_fflush() {
    FILE *fd = FileD(S0);
    fflush(fd);
    Pop;
}

// fd loc --
prim P_fseek() {
    int pos = S0;
    FILE *fd = FileD(S1);

    (void) fseek(fd,pos,SEEK_SET);
    Pop2;
}

prim P_i2cOpen() {
    I2C *i2c;

    i2c = new I2C(I2C_SDA, I2C_SCL);        // sda, scl

    Push = (stackitem)i2c;
}

prim P_i2cClose() {
    I2C *i2c;
    i2c = (I2C *)S0;
    i2c->~I2C();
    Pop;
}


prim P_i2cLock() {
    I2C *i2c;
    i2c = (I2C *)S0;
    i2c->lock();
    Pop;
}

prim P_i2cUnlock() {
    I2C *i2c;
    i2c = (I2C *)S0;
    i2c->unlock();
    Pop;
}
//
// Stack - ptr len i2c_addr i2c_instance--
//
prim P_i2cRead() {
    I2C *i2c;
    i2c = (I2C *)S0;

    int i2cAddr = (int)S1;
    int len = (int)S2;
    char *ptr=(char *)S3;

    i2c->read(i2cAddr,ptr,len);

    Npop(4);
}
//
// Stack - ptr len i2c_addr i2c_instance--
//
prim P_i2cWrite() {
    I2C *i2c;
    i2c = (I2C *)S0;

    int i2cAddr = (int)S1;
    int len = (int)S2;
    char *ptr=(char *)S3;

    i2c->write(i2cAddr,ptr,len);

    Npop(4);
}

prim P_i2cScan() {
    int ack;
    int address;
    char buffer[32];

    I2C *i2c;
    i2c = (I2C *)S0;

    i2c->lock();
    for(address=1;address<127;address++) {
        sprintf(buffer,"%02x\b\b", address);
        atlastTxString((char *)buffer);

        ack = i2c->write(address, "11", 1);
        if (ack == 0) {
            sprintf(buffer,"\r\n\tFound at %3d -- 0x%02x\r\n", address,address);
            atlastTxString((char *)buffer);
        }
        ThisThread::sleep_for(200);
    }
    i2c->unlock();

    /*
    cmd[0]=0;
    i2c.write(address, cmd,1);

   cmd[0]=0xff;
    i2c.write(address, cmd,1);
    */
}

prim P_ms() {
    int delay = (int)S0;

    ThisThread::sleep_for(delay);

    Pop;
}

// EYEWINK 6 LEDs & 6 Buttons


prim LED_cls() {
    EYEWINK.cls();
}

prim LED_write() {
    uint8_t d = (uint8_t) S0;
    uint8_t a = (uint8_t)S1;

    EYEWINK.writeData(d,a);

    Pop2;
}

prim LED_writeData() {
    char *p = (char *)S0;

    EYEWINK.writeData(p);

    Pop;

}

prim LED_type() {
    char *p = (char *)S0;

    EYEWINK.printf( p );

    Pop;
}

// Report if a key pressed.
//
prim LED_key() {
    TM1637::KeyData_t keydata;

    EYEWINK.getKeys(&keydata);

    Push = (stackitem) keydata;
}

prim LED_brightness() {
    uint8_t bright = (uint8_t)S0;

    EYEWINK.setBrightness(bright & 0x07);

    Pop;
}

prim LED_on() {
    EYEWINK.setDisplay(true);
}

prim LED_off() {
    EYEWINK.setDisplay(false);
}
//
// Move 'cursor' to position, 0 is extreme left.
// Stack: n --
//
prim LED_locate() {
    int l = (int) S0;
    EYEWINK.locate(l);

    Pop;
}





