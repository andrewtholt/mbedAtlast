#include "mbed.h"
#include "io.h"

extern Serial *pc;

extern "C" {

    void atlastTxByte(char c) {
        pc->putc(c);
    }

    void atlastTxString(char *ptr) {
        int len = strlen(ptr);

        for(int i=0; i<len; i++) {
            atlastTxByte( ptr[i]);
        }
    }

    void atlastTxBuffer(char *ptr, int len) {
        for(int i=0; i<len; i++) {
            atlastTxByte( ptr[i]);
        }
    }

};
