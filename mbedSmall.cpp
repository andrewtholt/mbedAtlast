#include <cstdint>
#include <string>
#include "mbed.h"
#include "mbedSmall.h"
#include "msg.h"
#include "parseMsg.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool; 

void mbedSmall::sendSet(uint8_t id,std::string key, std::string value) {
    parseMsg *p = new parseMsg((Small *)&db);;

}


