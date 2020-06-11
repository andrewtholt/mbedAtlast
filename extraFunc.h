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
//    {NULL, (codeptr) 0}
    {nullptr, (codeptr) 0}
};

