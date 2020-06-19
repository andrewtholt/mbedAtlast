#include "mbedSmall.h"
#include "parseMsg.h"
#include "mbed.h"
#include "io.h"
#include "PCF8574.h"
extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool;
extern Mutex stdio_mutex;

// I2C i2c(I2C_SDA, I2C_SCL);        // sda, scl

void i2cScan() {
    PCF8574 io(I2C_SDA,I2C_SCL,0x40);

    while(1) {
        io.write(0x0);
        ThisThread::sleep_for(200);
        io.write(0xF);
        ThisThread::sleep_for(200);
    }

    char buffer[16];
    stdio_mutex.lock();
    atlastTxString((char *)"i2cThread scan started\n");
    stdio_mutex.unlock();

    io.write(0x0);

    ThisThread::sleep_for(200);
    io.write(0xF);

    ThisThread::sleep_for(200);
/*
    for(int i = 63;i < 128 ; i++) {
        sprintf(buffer,"0x%x..",i);
        atlastTxString(buffer);

        i2c.start();
        if((i2c.write(i << 1)) == 0) {
            sprintf(buffer,"0x%x ACK \r\n",i);
//            atlastTxString(buffer);
//            pc.printf("0x%x ACK \r\n",i); // Send command strin
        }
        i2c.stop();
        ThisThread::sleep_for(30);
    }
        */
}

void i2cTask(void) {

    uint32_t dly = osWaitForever;
    mbedSmall *db = new mbedSmall();
    parseMsg *p = new parseMsg( db );
    bool runFlag = true;

    //
    // Change this to a valid task id, in tasks.h
    //
    taskId iam = taskId::I2C;

    i2cScan();

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
