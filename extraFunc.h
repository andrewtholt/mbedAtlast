extern "C" {
#include "atldef.h"
}
void crap();
void cpp_extrasLoad();
void mkMessage();
void setSender();
void setTopic();
void setMsg();
void sendMsg();

void getMainId();
void getLEDId();

void getLowLevelType();
void getHighLevelType();

void MBED_addRecord();
void MBED_lookup();

void MBED_subscribe();
void MBED_get();

void MBED_dbToMsg();
void MBED_msgDump();

static struct primfcn cpp_extras[] = {
    {(char *)"0TESTING", crap},

    {(char *)"0HI@", getHighLevelType},
    {(char *)"0LO@", getLowLevelType},
    {(char *)"0MKMSG", mkMessage},
    {(char *)"0SET-SENDER", setSender},
    {(char *)"0SET-TOPIC", setTopic},
    {(char *)"0SET-MSG", setMsg},
    {(char *)"0SEND-MSG", sendMsg},
    {(char *)"0MAIN@", getMainId},
    {(char *)"0LED@", getLEDId},

    {(char *)"0ADD-RECORD",  MBED_addRecord},
    {(char *)"0LOOKUP",  MBED_lookup},
    {(char *)"0SUBSCRIBE", MBED_subscribe},
    {(char *)"0GET", MBED_get},
    {(char *)"0DB-TO-MSG",  MBED_dbToMsg},
    {(char *)"0MSG-DUMP",  MBED_msgDump},

//    {NULL, (codeptr) 0}
    {nullptr, (codeptr) 0}
};


