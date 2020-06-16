/***********************************************************************
 * AUTHOR: andrewh <andrewh>
 *   FILE: .//parseMsg.cpp
 *   DATE: Mon Jun 15 15:21:01 2020
 *  DESCR: 
 ***********************************************************************/
#include "parseMsg.h"
#include "msg.h"
extern "C" {
#include <string.h>
};
/***********************************************************************
 *  Method: parseMsg::parseMsg
 *  Params: Small *db
 * Effects: 
 ***********************************************************************/
parseMsg::parseMsg(Small *db) {
    data = db;
}


/***********************************************************************
 *  Method: parseMsg::~parseMsg
 *  Params: 
 * Effects: 
 ***********************************************************************/
parseMsg::~parseMsg() {
}


/***********************************************************************
 *  Method: parseMsg::fromMsgToDb
 *  Params: message_t *msg
 * Returns: bool
 * Effects: 
 ***********************************************************************/
bool parseMsg::fromMsgToDb(message_t *msg) {
    bool fail=true;

    msgType t = msg->type;

    if( t == msgType::HI_LEVEL) {
        if( msg->op.hl_op == highLevelOperation::SET) {
            char *key = msg->body.hl_body.topic;
            char *value = msg->body.hl_body.msg;

            data->Set(key,value);
            fail = false;
        }
    }

    return fail;
}


/***********************************************************************
 *  Method: parseMsg::fromDbToMsg
 *  Params: message_t *msg
 * Returns: bool
 * Effects: 
 ***********************************************************************/
bool parseMsg::fromDbToMsg(message_t *msg,taskId sender, char *key) {
    bool fail=true;

    std::string v;

    v = data->Get( key );

    if( v != "<NON>" ) {
        msg->Sender = sender;
        msg->type = msgType::HI_LEVEL;
        msg->op.hl_op = highLevelOperation::SET;
        strncpy(msg->body.hl_body.topic, key, MAX_TOPIC);
        strncpy(msg->body.hl_body.msg, v.c_str(), MAX_MSG);

        fail=false;
    }

    return fail;
}

bool parseMsg::subscribe(message_t *msg, taskId sender, char *key) {
    bool fail=true;
    
    std::string v = data->Get( key );

    if( v != "<NON>" ) {
        msg->Sender = sender;
        msg->type = msgType::HI_LEVEL;
        msg->op.hl_op = highLevelOperation::SUB;
        strncpy(msg->body.hl_body.topic, key, MAX_TOPIC);
//        strncpy(msg->body.hl_body.msg, v.c_str(), MAX_MSG);

        fail=false;
    }
    
    return fail;
}

taskId parseMsg::getSender(message_t *msg) {
    return msg->Sender;
}

char *parseMsg::getKey(message_t *msg) {
    return (char *)msg->body.hl_body.topic;
}
