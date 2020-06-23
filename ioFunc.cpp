#include "mbed.h"
#include <stdio.h>
#include <errno.h>

#include "SDBlockDevice.h"
#include "LittleFileSystem.h"

#include "ioFunc.h"

extern LittleFileSystem *fileSystem;
extern "C" {
//    #include "atldef.h"
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

/* Open file: fname fmodes fd -- flag */
prim P_fopen()	{
    stackitem stat;

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
//
// File read: buf len fd -- length */
//
prim P_fread() {
    //    Sl(3);
    //    Hpc(S0);

    //     Isfile(S0);
    //    Isopen(S0);

    FILE *fd = FileD(S0);
    int len = (int)S1;
    void *buffer = (void *)S2;

    size_t ret= fread(buffer, 1,len, fd);

    Pop2;

    S0 = (stackitem) ret;
}

// Get line: fd string -- flag
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
    (void) fclose(FileD(S0));

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

    if(!fd) {
        S0=(stackitem)-1;
    } else {
        size_t ret= fread(buffer, 1,sizeof(buffer), fd);


    }

}

prim FS_ls() {
    int err;


    DIR *d = opendir("/fs/");
    if (!d) {
        error("error: %s (%d)\r\n", strerror(errno), -errno);
    }
    /*
    DIR *d = opendir("/fs/");
    if (!d) {
        error("error: %s (%d)\r\n", strerror(errno), -errno);
    }

    while (true) {
        struct dirent*  e = readdir(d);
        if (!e) {
            break;
        }

        printf("    %s\r\n", e->d_name);
    }
    err = closedir(d);
    */
}

void io_extrasLoad() {
    atl_primdef( fileioExtras );
}

