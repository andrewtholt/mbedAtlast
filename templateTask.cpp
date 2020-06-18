#include "mbedSmall.h"
#include "parseMsg.h"
#include "mbed.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool;

void templateTask(void) {

    uint32_t dly = osWaitForever;
    mbedSmall *db = new mbedSmall();
    parseMsg *p = new parseMsg( db );
    bool runFlag = true;

    // 
    // Change this to a valid task id, in tasks.h
    //
    taskId iam = taskId::INVALID;

    while(runFlag) {
        osEvent evt = tasks[(int)iam].get( dly );

        if(evt.status == osEventMessage) {
            message_t *message = (message_t*)evt.value.p;

            switch(message->op.hl_op) {
                case highLevelOperation::NOP :
                    break;
                case highLevelOperation::GET :
                    {
                        char *k = p->getKey(message);
                        taskId to = p->getSender(message) ;

                        std::string v=db->Get( k );
                        db->sendSet(iam, to,k, v.c_str()) ;
                    }
                    break;
                case highLevelOperation::SET :
                    break;
                case highLevelOperation::SUB :
                    {
                        taskId id = p->getSender(message);
                        char *k = p->getKey(message);

                        db->Sub(k,(uint8_t)id);
                    }
                    break;
                case highLevelOperation::UNSUB :
                    break;
            }

            mpool.free(message);
        }
    }
}
