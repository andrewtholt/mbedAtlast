#include <cstdint>
#include <string>
#include "mbed.h"
#include "mbedSmall.h"
#include "msg.h"
#include "io.h"
#include "tasks.h"
#include "parseMsg.h"
#include "atlastUtils.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool;
extern Mutex stdio_mutex;

void mbedSmall::sendSet(taskId source, taskId dest, std::string key, std::string value) {
    parseMsg *p = new parseMsg((Small *)&db);

    message_t *msg =mpool.calloc();

    bool fail = p->fromDbToMsg( msg, source, (char *)key.c_str());

    /*
    stdio_mutex.lock();

    atlastTxString((char *)"\nmbedSmall::sendSet\n");
    msgDump(msg);

    stdio_mutex.unlock();
    */

    osStatus res=tasks[(int)dest].put(msg);

    switch (res) {
        case osOK:
            break;
        case osErrorResource:
            stdio_mutex.lock();
            atlastTxString((char *)"\nmbedSmall::sendSet queue full\n");
//            msgDump(msg);
            stdio_mutex.unlock();
            break;
    }
    delete p;
}
