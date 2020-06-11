extern "C" {
#include "atldef.h"
}
void crap();
void cpp_extrasLoad();


static struct primfcn cpp_extras[] = {
    {"0TESTING", crap},
//    {NULL, (codeptr) 0}
    {nullptr, (codeptr) 0}
};


