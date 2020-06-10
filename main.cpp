#include "mbed.h"
extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "atlast.h"
#include "atlcfig.h"
#include "atldef.h"

#include "extraFunc.h"

// uint8_t nvramrc[] = ": tst \n10 0 do \ni . cr \nloop \n; \n \n";
uint8_t nvramrc[] = ": sifting\ntoken $sift\n;\n: [char] char ;";
//

char outBuffer[OUTBUFFER];

int main() {
    char t[132];
    int8_t len;

    bool runFlag=true;

    uint8_t lineBuffer[MAX_LINE];
//    dictword *var;
//    int *tst;

    atl_init();

    do {
        memset(lineBuffer,0,MAX_LINE);
        len=readLineFromArray(nvramrc,lineBuffer);
        atl_eval((char *)lineBuffer);
    } while(len >= 0);

//    var = atl_vardef("TEST",4);

    /*
    if(var == NULL) {
        fprintf(stderr,"Vardef failed\n");
    } else {
        *((int *)atl_body(var))=42;
    }
    */

    cpp_extrasLoad();
    while(runFlag) {
        (void)memset(outBuffer,0,sizeof(outBuffer));
        printf("-> ");

        (void)fgets(t,132,stdin);
        atl_eval(t);

    }
    return 0;
}
}
