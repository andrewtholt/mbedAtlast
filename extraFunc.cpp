using namespace std;
#include "mbed.h"

extern "C" {
    #include "extraFunc.h"
    #include "atldef.h"
}


prim crap() {
}


void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

