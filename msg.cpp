#include "msg.h"
#include "tasks.h"
#include <string>
#include <map>

extern "C" {
#include <string.h>
}

void mkSubMsg(message_t *msg, taskId sender, char *key) {
    msg->Sender = sender;
    msg->type = msgType::HI_LEVEL;
    msg->op.hl_op = highLevelOperation::SUB;
    strncpy(msg->body.hl_body.topic, key, MAX_TOPIC);
}

void mkGetMsg(message_t *msg, taskId sender, char *key) {
    msg->Sender = sender;
    msg->type = msgType::HI_LEVEL;
    msg->op.hl_op = highLevelOperation::GET;
    strncpy(msg->body.hl_body.topic, key, MAX_TOPIC);
}

void mkSetMsg(message_t *msg, taskId sender, char *key, char *value) {
    msg->Sender = sender;
    msg->type = msgType::HI_LEVEL;
    msg->op.hl_op = highLevelOperation::SET;
    strncpy(msg->body.hl_body.topic, key, MAX_TOPIC);
    strncpy(msg->body.hl_body.msg, value, MAX_MSG);
}
