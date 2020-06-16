#pragma once
#include "msg.h"
// #include "Small.h"
#include "mbedSmall.h"
#include "tasks.h"

class parseMsg {
    private:
        Small *data;
    public:
        parseMsg(Small *db);
        ~parseMsg();

        bool fromMsgToDb(message_t *msg);
        bool fromDbToMsg(message_t *msg,taskId sender, char *key);
        bool subscribe(message_t *msg, taskId sender, char *key) ;
        
        taskId getSender(message_t *msg) ;
        char *getKey(message_t *msg) ;
        
};

