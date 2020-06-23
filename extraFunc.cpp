
#include "mbed.h"
#include "msg.h"
#include "tasks.h"
#include "SDBlockDevice.h"
#include "LittleFileSystem.h"
#include <stdio.h>
#include <errno.h>

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
    #include "atldef.h"
}

using namespace std;

prim crap() {
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
    Push = (int) taskId::LED_CTRL;
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

    bool fail = parse->mkSubMsg( msg, sender, key);
    if(fail) {
        mpool.free(msg);
    }

    Npop(3);
    S0 = (stackitem)fail;
}

prim MBED_get() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    parse->mkGetMsg( msg, sender, key);
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


