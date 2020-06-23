#pragma once

extern "C" {
#include <stdio.h>
#include "atldef.h"
#include "atlcfig.h"
}

#ifdef FILEIO

void P_fopen();
void P_fread();
void P_fwrite();
void P_fgetline();
void P_fflush();
void P_fseek();
void P_fclose();

void FS_remove();
void FS_ls();
void FS_cat();

static struct primfcn fileioExtras[]= {
    {(char *)"0FOPEN", P_fopen},
    {(char *)"0FREAD", P_fread},
    {(char *)"0FWRITE", P_fwrite},
    {(char *)"0FGETLINE", P_fgetline},
    {(char *)"0FSEEK", P_fseek},
    {(char *)"0FFLUSH", P_fflush},
    {(char *)"0FCLOSE", P_fclose},

    {(char *)"0REMOVE", FS_remove},
    {(char *)"0LS", FS_ls},
    {(char *)"0$CAT", FS_cat},

    {nullptr, (codeptr) 0}
};
#endif

