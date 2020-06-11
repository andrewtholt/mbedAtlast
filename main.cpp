#include "mbed.h"
#define ECHO
extern "C" {
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "atlast.h"
#include "atlcfig.h"
#include "atldef.h"

#include "extraFunc.h"

#ifdef NVRAMRC
#include "nvramrc.h"
#else
    uint8_t nvramrc[] = ": tst \n10 0 do \ni . cr \nloop \n; \n \n";
    // uint8_t nvramrc[] = ": sifting\ntoken $sift\n;\n: [char] char ;";
#endif

    char outBuffer[OUTBUFFER];

    int getline(Serial *port, uint8_t *b, const uint8_t len) {

        bool done=false;
        uint8_t count=0;
        char c;

        while ( done == false ) {
            c=port->getc();
#ifdef ECHO
            port->putc(c);
#endif

            if(iscntrl(c)) {
                switch(c) {
                    case '\n':
                    case '\r':
                        b[count] = '\0';
                        done=true;
                        break;
                    case 0x08:
//                        port->putc(0x08);
                        port->putc(' ');
                        port->putc(0x08);
                        count--;
                        b[count] = '\0';
                        break;
                }
            } else {
                b[count++] = c;
            }
        }
        return(count);
    }

    int main() {
        char t[132];
        int8_t len;
        //    Serial pc(USBTX, USBRX);

        Serial *pc = new Serial(USBTX, USBRX);

        bool runFlag=true;

        uint8_t lineBuffer[MAX_LINE];
        dictword *var;
        //    int *tst;

        pc->baud(115200);

        atl_init();

        do {
            memset(lineBuffer,0,MAX_LINE);
            len=readLineFromArray(nvramrc,lineBuffer);
            atl_eval((char *)lineBuffer);
        } while(len >= 0);

       var = atl_vardef("FOUR",4);

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
            (void)memset((char *)lineBuffer,0,sizeof(lineBuffer));
            pc->printf("\n\r-> ");

            len = getline(pc, lineBuffer, MAX_LINE) ;
//            (void)pc->gets(t,132);
            atl_eval((char *)lineBuffer);

        }
        return 0;
    }
}
