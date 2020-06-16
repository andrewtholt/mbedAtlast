
#include "mbed.h"
#include "msg.h"
#include "tasks.h"
// #include "Small.h"
#include "mbedSmall.h"
#include "parseMsg.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool; 
extern Serial *pc;

char dataBuffer[255];  //Somewhat to put data.

using namespace std;

extern "C" {
    #include "extraFunc.h"
    #include "atldef.h"
}

prim crap() {
}
// 
// stack: <msg> <task id> --
//
prim sendMsg() {
    int dest = (int) S0;
    message_t *out = (message_t *)S1;

    tasks[dest].put(out);

    Pop2;
}

prim getLowLevelType() {
    Push = (int) msgType::LO_LEVEL;
}

prim getHighLevelType() {
    Push = (int) msgType::HI_LEVEL;
}

prim getMainId() {
    Push = (int) taskId::ATLAST;
}

prim getLEDId() {
    Push = (int) taskId::LED_CTRL;
}

prim mkMessage() {
    message_t *msg = mpool.calloc();
    Push = (stackitem) msg;
}

prim setSender() {
    message_t *msg = (message_t *)S0;

    msg->Sender = (taskId)S1;

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
    
    bool fail = parse->subscribe( msg, sender, key);
    if(fail) {
        mpool.free(msg);    
    }
    
    Npop(3);
    S0 = (stackitem)fail;
}

void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

