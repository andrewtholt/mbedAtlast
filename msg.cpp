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

void msgToRemote(message_t *msg, uint8_t *ptr) {

    uint8_t packetLen = (uint8_t) strlen((char *)ptr);
    int offset=0;

    if ( msg->type == msgType::HI_LEVEL) {

        ptr[offset++] = '*';
        ptr[offset++] = 0xff;               // Offset = 1 is the length of the message, fix later

        ptr[offset++] = (uint8_t) highLevelOperation::SET;
        ptr[offset++] = (uint8_t) 4;
        strncpy((char *)&ptr[offset],(char *)"NONE", 4);
        offset += 4;

        uint8_t topicLen = (uint8_t)strlen( msg->body.hl_body.topic);
        ptr[offset++] = topicLen;
        strncpy((char *)&ptr[offset], msg->body.hl_body.topic, topicLen);
        offset += topicLen;

        uint8_t valueLen = strlen(msg->body.hl_body.msg);
        ptr[offset++] = valueLen;
        strncpy((char *)&ptr[offset], msg->body.hl_body.msg, valueLen);

        ptr[1] = strlen((char *)ptr);
    }

}

