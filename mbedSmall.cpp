#include <cstdint>
#include <string>
#include "mbed.h"
#include "mbedSmall.h"
#include "msg.h"
#include "parseMsg.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool; 

void mbedSmall::sendSet(uint8_t id,std::string key, std::string value) {
    volatile uint8_t x = id;
    parseMsg *p = new parseMsg((Small *)&db);
    
    message_t *msg =mpool.calloc();
    
    bool fail = p->fromDbToMsg( msg, (taskId)id, (char *)key.c_str());
    
    tasks[id].put(msg);
    
    delete p;
}
