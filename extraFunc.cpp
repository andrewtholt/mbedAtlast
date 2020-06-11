
#include "mbed.h"
#include "msg.h"
#include "tasks.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool; 
extern Serial *pc;

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
    Push = (int) taskId::MAIN;
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


void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

