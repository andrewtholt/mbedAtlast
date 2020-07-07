extern "C" {
#include "atldef.h"
}
void crap();
void P_setRemoteCommand() ;

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
void taskQdepth() ;

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

void P_listDirectory();
void P_dCat() ;
void P_dRm() ;
void P_dTouch() ;

void P_catFile() ;
void P_rm() ;
void P_touch() ;
void P_DL();

void P_fclose();
void P_i2cScan();
void P_i2cOpen();
void P_i2cLock();
void P_i2cUnlock();

void P_i2cWrite();
void P_i2cRead();
void P_i2cClose();

void P_ms();

// EYEWINK LED display.
void LED_cls() ;
void LED_write() ;
void LED_type() ;
prim LED_key() ;
prim LED_brightness() ;
prim LED_on() ;
prim LED_off() ;
prim LED_locate() ;

static struct primfcn cpp_extras[] = {
    {(char *)"0TESTING", crap},
    {(char *)"0REMOTE-CMD", P_setRemoteCommand},

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

    {(char *)"0MAIN-TID", getMainId},
    {(char *)"0LED-TID", getLEDId},
    {(char *)"0I2C-TID", getI2CId},

    {(char *)"0I2C-OPEN", P_i2cOpen},
    {(char *)"0I2C-LOCK", P_i2cLock},
    {(char *)"0I2C-UNLOCK", P_i2cUnlock},
    {(char *)"0I2C-SCAN", P_i2cScan},
    {(char *)"0I2C-WRITE", P_i2cWrite},
    {(char *)"0I2C-READ", P_i2cRead},
    {(char *)"0I2C-CLOSE", P_i2cClose},

    {(char *)"0MS", P_ms},
    {(char *)"0QDEPTH", taskQdepth},
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

    {(char *)"0LS", P_listDirectory},
    {(char *)"0$CAT", P_dCat},
    {(char *)"0$RM", P_dRm},
    {(char *)"0$TOUCH", P_dTouch},

    {(char *)"1CAT", P_catFile},
    {(char *)"1RM", P_rm},
    {(char *)"1TOUCH", P_touch},
    {(char *)"1DL", P_DL},

    {(char *)"0FCLOSE", P_fclose},

    {(char *)"0LED-CLS", LED_cls},
    {(char *)"0LED-WRITE", LED_write},
    {(char *)"0LED-TYPE", LED_type},
    {(char *)"0LED-KEY", LED_key},
    {(char *)"0LED-BRIGHT", LED_brightness},
    {(char *)"0LED-ON", LED_on},
    {(char *)"0LED-OFF", LED_off},
    {(char *)"0LED-LOC", LED_locate},

//    {NULL, (codeptr) 0}
    {nullptr, (codeptr) 0}
};

void LED_cls() ;
void LED_write() ;
