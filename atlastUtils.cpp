// #include "utils.h"

#include "msg.h"
#include "io.h"
#include "rtos.h"

extern Mutex stdio_mutex;

void printIam(taskId iam) {

//    osThreadId_t me = ThisThread::get_id();

    atlastTxString((char *)"\r\nI am   : ");
    switch(iam) {
        case taskId::INVALID:
            atlastTxString((char *)"INVALID\r\n");
            break;
        case taskId::ATLAST:
            atlastTxString((char *)"ATLAST\r\n");
            break;
        case taskId::LED_CTRL:
            atlastTxString((char *)"LED_CTRL\r\n");
            break;
        case taskId::CTRL:
            atlastTxString((char *)"CTRL\r\n");
            break;
        case taskId::I2C:
            atlastTxString((char *)"I2C\r\n");
            break;
        case taskId::LAST:
            atlastTxString((char *)"LAST\r\n");
            break;
    }
}

void msgDump(message_t *msg) {
    atlastTxString((char *)"\r\n\r\n");

    atlastTxString((char *)"Sender : ");
    taskId s = msg->Sender;

    switch(s) {
        case taskId::INVALID:
            atlastTxString((char *)"INVALID\r\n");
            break;
        case taskId::ATLAST:
            atlastTxString((char *)"ATLAST\r\n");
            break;
        case taskId::LED_CTRL:
            atlastTxString((char *)"LED_CTRL\r\n");
            break;
        case taskId::CTRL:
            atlastTxString((char *)"CTRL\r\n");
            break;
        case taskId::I2C:
            atlastTxString((char *)"I2C\r\n");
            break;
        case taskId::LAST:
            atlastTxString((char *)"LAST\r\n");
            break;
    }


    atlastTxString((char *)"Type   : ");
    msgType t = msg->type;

    switch(t) {
        case msgType::INVALID:
            atlastTxString((char *)"INVALID\r\n");
            break;
        case msgType::HI_LEVEL:
            atlastTxString((char *)"HI_LEVEL\r\n");
            break;
        case msgType::LO_LEVEL:
            atlastTxString((char *)"LO_LEVEL\r\n");
            break;
    }

    if( t == msgType::HI_LEVEL) {
        bool printTopic = true;
        bool printMsg = false;

        highLevelOperation op = msg->op.hl_op;

        atlastTxString((char *)"Op     : ");

        switch(op) {
            case highLevelOperation::NOP:
                atlastTxString((char *)"NOP\r\n");
                printTopic = false;
                printMsg = false;
                break;
            case highLevelOperation::GET:
                atlastTxString((char *)"GET\r\n");
                printTopic = true;
                printMsg = false;
                break;
            case highLevelOperation::SET:
                atlastTxString((char *)"SET\r\n");
                printTopic = true;
                printMsg = true;
                break;
            case highLevelOperation::SUB:
                atlastTxString((char *)"SUB\r\n");
                printTopic = true;
                printMsg = false;
                break;
            case highLevelOperation::UNSUB:
                atlastTxString((char *)"UNSUB\r\n");
                printTopic = true;
                printMsg = false;
                break;
        }

        if( printTopic ) {
            atlastTxString((char *)"Topic  : ");
            atlastTxString( msg->body.hl_body.topic);
            atlastTxByte('\r\n');
        }

        if( printMsg ) {
            atlastTxString((char *)"Msg    : ");
            atlastTxString( msg->body.hl_body.msg);
            atlastTxByte('\r\n');
        }
    }
}
