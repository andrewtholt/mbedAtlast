
#include "mbed.h"
#include "msg.h"
#include "tasks.h"
#include "SDBlockDevice.h"
#include "LittleFileSystem.h"
#include <stdio.h>
#include <errno.h>

/*
 * SDBlockDevice blockDevice(PA_7, PA_6, PA_5, PA_8);
 * LittleFileSystem fileSystem("fs");
 */
extern SDBlockDevice *blockDevice;
extern LittleFileSystem *fileSystem;

// #include "Small.h"
#include "mbedSmall.h"
#include "parseMsg.h"

#include "io.h"
#include "tasks.h"
#include "atlastUtils.h"

extern Queue<message_t, 8> tasks[];
extern MemoryPool<message_t, 8> mpool;
extern Serial *pc;
extern Mutex stdio_mutex;

char dataBuffer[255];  //Somewhat to put data.


extern "C" {
    #include "extraFunc.h"
    #include "atldef.h"
}

char *atl_fgetsp(char *s, int n, FILE *stream) {
    int i = 0, ch;

    while (True) {
        ch = getc(stream);
        if (ch == EOF) {
            if (i == 0)
                return NULL;
            break;
        }
        if (ch == '\r') {
            ch = getc(stream);
            if (ch != '\n')
                (void) ungetc(ch, stream);
            break;
        }
        if (ch == '\n') {
            ch = getc(stream);
            if (ch != '\r')
                (void) ungetc(ch, stream);
            break;
        }
        if (i < (n - 1))
            s[i++] = ch;
    }
    s[i] = 0;
    return s;
}


using namespace std;

prim crap() {
}
//
// stack: <task-id> <msg> --
//
prim sendMsg() {

    int dest = (int) S1;
    message_t *out = (message_t *)S0;

    tasks[dest].put(out);

    Pop2;
}

prim getLowLevelType() {
    Push = (int) msgType::LO_LEVEL;
}

prim getHighLevelType() {
    Push = (int) msgType::HI_LEVEL;
}

prim opNop() {
    Push = (int) highLevelOperation::NOP;
}

prim opGet() {
    Push = (int) highLevelOperation::GET;
}

prim opSet() {
    Push = (int) highLevelOperation::SET;
}

prim opSub() {
    Push = (int) highLevelOperation::SUB;
}

prim opUnsub() {
    Push = (int) highLevelOperation::UNSUB;
}

prim getMainId() {
    Push = (int) taskId::ATLAST;
}

prim getLEDId() {
    Push = (int) taskId::LED_CTRL;
}

prim getI2CId() {
    Push = (int) taskId::I2C;
}

prim mkMessage() {
    message_t *msg = mpool.calloc();
    Push = (stackitem) msg;
}
//
// Stack : Sender msg --
//
prim setSender() {
    message_t *msg = (message_t *)S0;

    msg->Sender = (taskId)S1;

    Pop2;
}
//
// Stack : type msg --
//
prim setType() {
    message_t *msg = (message_t *)S0;

    msg->type = (msgType)S1;

    Pop2;
}
//
// Stack : type msg --
//
prim setOp() {
    message_t *msg = (message_t *)S0;

    msg->op.hl_op = (highLevelOperation)S1;

    Pop2;
}

prim setTopic() {
    message_t *msg = (message_t *)S0;
    char *topic = (char *)S1;

    strcpy(msg->body.hl_body.topic,topic);

    Pop2;
}

prim setMsg() {
    message_t *msg = (message_t *)S0;

    char *payload = (char *)S1;

    strcpy(msg->body.hl_body.msg,payload);

    Pop2;
}

//
// Stack: db key value --
//
prim MBED_addRecord() {
    //    Sl(3);
    char *value =(char *)S0;
    char *key = (char *)S1;
    Small *db = (Small *)S2;

    db->Set(key, value);
    Pop2;
    Pop;
    //    So(0);
}
//
// Stack: db key --
prim MBED_lookup() {
    char *key = (char *)S0;
    Small *db = (Small *)S1;

    string tmp = db->Get(key);
    strcpy(dataBuffer, tmp.c_str());
    Pop;
    S0=(stackitem)dataBuffer;
}
//
// Find a record in the db and populate a message.
//
prim MBED_dbToMsg() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    bool fail = parse->fromDbToMsg( msg, sender, key);
    if(fail) {
        mpool.free(msg);
    }

    Npop(3);
    S0 = (stackitem)fail;
}

prim MBED_subscribe() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    bool fail = parse->mkSubMsg( msg, sender, key);
    if(fail) {
        mpool.free(msg);
    }

    Npop(3);
    S0 = (stackitem)fail;
}

prim MBED_get() {
    parseMsg *parse = (parseMsg *)S3;
    message_t *msg = (message_t *)S2;
    taskId sender = (taskId)S1;
    char *key = (char *)S0;

    memset(msg,0,sizeof(message_t));

    parse->mkGetMsg( msg, sender, key);
    msg->op.hl_op = highLevelOperation::GET;

    Npop(4);
}

prim MBED_msgDump() {
    message_t *msg = (message_t *)S0;

    stdio_mutex.lock();
    msgDump( msg );
    stdio_mutex.unlock();

    Pop;
}

prim subscribers() {
}

void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

void initFs() {
    atlastTxString((char *)"\r\nSetup filesystem\r\n");
    blockDevice = new SDBlockDevice (PA_7, PA_6, PA_5, PA_8);
    fileSystem = new LittleFileSystem("fs");

    atlastTxString((char *)"Mounting the filesystem... \r\n");

    int err = fileSystem->mount(blockDevice);

    if(err) {
        atlastTxString((char *)"\r\nNo filesystem found, formatting...\r\n");
        err = fileSystem->reformat(blockDevice);

        if(err) {
            error("error: %s (%d)\r\n", strerror(-err), err);
        } else {
            atlastTxString((char *)"... done.\r\n");
        }
    } else {
        atlastTxString((char *)"... done.\r\n");
    }

    FILE *fd=fopen("/fs/numbers.txt", "r+");
    if(!fd) {
        atlastTxString((char *)" failed to open file.\r\n");
    } else {
        atlastTxString((char *)"closing file.\r\n");
        fclose(fd);
    }
}

/* Open file: fname fmodes fd -- flag */
prim P_fopen()	{
    stackitem stat;

    //    Sl(3);
    //    Hpc(S2);
    //    Hpc(S0);

    //    Isfile(S0);

    /*
     *    FILE *fd=fopen("/fs/numbers.txt", "r+");
     *    if(!fd) {
     *        atlastTxString((char *)" failed to fopen file.\r\n");
} else {
    fclose(fd);
}
*/

    char * fname = (char *) S2;
    char *mode =(char *)S1;

    FILE *fd = fopen(fname,mode);

    if (fd == NULL) {
        stat = false;
    } else {
        *(((stackitem *) S0) + 1) = (stackitem) fd;
        stat = true;
    }

    Pop2;
    S0 = stat;
}

/* Was ------- File read: fd len buf -- length */
/* ATH Now --- File read: buf len fd -- length */
prim P_fread() {
    /* Was ------- File read: fd len buf -- length */
    /* ATH Now --- File read: buf len fd -- length */
    //    Sl(3);
    //    Hpc(S0);

    //     Isfile(S0);
    //    Isopen(S0);
    // TODO This is stupid, it is inconsisitent with write.
    // The stack order should follow the C convention.
    // S2 = fread((char *) S0, 1, ((int) S1), FileD(S2));
    //    S2 = fread((char *) S2, 1, ((int) S1), FileD(S0));

    FILE *fd = FileD(S0);
    int len = (int)S1;
    void *buffer = (void *)S2;

    //    size_t ret= fread(buffer, len,1, fd);
    size_t ret= fread(buffer, 1,len, fd);

    Pop2;

    S0 = (stackitem) ret;
}

prim P_fgetline() {
    //    Sl(2);
    //    Hpc(S0);
    //    Isfile(S1);
    //    Isopen(S1);

    if (atl_fgetsp((char *) S0, 132, FileD(S1)) == NULL) {
        S1 = 0;
    } else {
        S1 = -1;
    }
    Pop;
}

/* Close file: fd -- */
prim P_fclose() {
    //    Sl(1);
    //    Hpc(S0);

    //    Isfile(S0);
    //    Isopen(S0);
    V fclose(FileD(S0));

    *(((stackitem *) S0) + 1) = (stackitem) NULL;
    Pop;
}

// was : File write: len buf fd -- length
// Is : write: buff, size, fd -- length
prim P_fwrite() {
    //    Sl(3);
    //    Hpc(S2);
    //    Isfile(S0);

    //    Isopen(S0);

    FILE *fd = FileD(S0);
    int len = (int)S1;
    void *buffer = (void *)S2;

    size_t ret= fwrite(buffer, 1,len, fd);
    //    S2 = fwrite((char *) S1, 1, ((int) S2), FileD(S0));
    Pop2;

    S0 = (stackitem) ret;
}

prim P_fflush() {
    FILE *fd = FileD(S0);
    fflush(fd);
    Pop;
}

// fd loc --
prim P_fseek() {
    int pos = S0;
    FILE *fd = FileD(S1);

    (void) fseek(fd,pos,SEEK_SET);
    Pop2;
}

prim FS_remove() {
    char *fpath = (char *) S0;

    int err = fileSystem->remove(fpath);

    S0 = (err < 0) ? -1 : 0;

}

prim FS_cat() {
    char buffer[255];
    char *fname = (char *) S0;

    FILE *fd = fopen(fname,(char *)"r");
    size_t ret=-1;

    if(!fd) {
        S0=(stackitem)-1;
    } else {
        stdio_mutex.lock();
        do {
            ret= fread(buffer, 1,sizeof(buffer), fd);
            atlastTxString((char *)buffer);
            memset(buffer,0,sizeof(buffer));
        } while(ret > 0);
        stdio_mutex.lock();
    }

}

prim FS_ls() {
    int err;


    DIR *d = opendir("/fs/");
    if (!d) {
        error("error: %s (%d)\r\n", strerror(errno), -errno);
    }

    while (true) {
        struct dirent*  e = readdir(d);
        if (!e) {
            break;
        }

        stdio_mutex.lock();
        printf("    %s\r\n", e->d_name);
        stdio_mutex.lock();
    }
    err = closedir(d);

}



