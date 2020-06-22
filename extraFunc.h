extern "C" {
#include "atldef.h"
}
void crap();
void cpp_extrasLoad();
void mkMessage();
void setSender();
void setType();

void setTopic();
void setOp();
void setMsg();
void sendMsg();

void getMainId();
void getLEDId();

void getLowLevelType();
void getHighLevelType();

void opNop();
void opGet();
void opSet();
void opSub();
void opUnsub();

void MBED_addRecord();
void MBED_lookup();

void MBED_subscribe();
void MBED_get();

void MBED_dbToMsg();
void MBED_msgDump();
void getI2CId() ;
void subscribers();
void P_fopen();
void P_fread();
void P_fwrite();
void P_fflush();
void P_fseek();

void P_fclose();

static struct primfcn cpp_extras[] = {
    {(char *)"0TESTING", crap},

    {(char *)"0HI-LEVEL", getHighLevelType},
    {(char *)"0LO-LEVEL", getLowLevelType},
    {(char *)"0NOP", opNop},
    {(char *)"0GET", opGet},
    {(char *)"0SET", opSet},
    {(char *)"0SUB", opSub},
    {(char *)"0UNSUB", opUnsub},

    {(char *)"0MKMSG", mkMessage},

    {(char *)"0SET-SENDER", setSender},
    {(char *)"0SET-TYPE", setType},
    {(char *)"0SET-OP", setOp},
    {(char *)"0SET-TOPIC", setTopic},
    {(char *)"0SET-MSG", setMsg},

    {(char *)"0SEND-MSG", sendMsg},

    {(char *)"0MAIN@", getMainId},
    {(char *)"0LED@", getLEDId},
    {(char *)"0I2C@", getI2CId},
    //
    // database operations
    //
    {(char *)"0ADD-RECORD",  MBED_addRecord},
    {(char *)"0LOOKUP",  MBED_lookup},

    {(char *)"0SUBSCRIBE-MSG", MBED_subscribe},
    {(char *)"0GET-MSG", MBED_get},
    {(char *)"0DB-TO-MSG",  MBED_dbToMsg},
    {(char *)"0MSG-DUMP",  MBED_msgDump},

    {(char *)"0FOPEN", P_fopen},
    {(char *)"0FREAD", P_fread},
    {(char *)"0FWRITE", P_fwrite},
    {(char *)"0FSEEK", P_fseek},
    {(char *)"0FFLUSH", P_fflush},

    {(char *)"0FCLOSE", P_fclose},
//    {NULL, (codeptr) 0}
    {nullptr, (codeptr) 0}
};


