#include "mbed.h"
#include "io.h"

extern Serial *pc;

extern "C" {

    void usbTxByte(char c) {
        pc->putc(c);
    }

    void usbTxString(char *ptr) {
        int len = strlen(ptr);

        for(int i=0; i<len; i++) {
            usbTxByte( ptr[i]);
        }
    }

};
