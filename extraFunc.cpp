using namespace std;

extern "C" {
    #include "atldef.h"
    #include "extraFunc.h"
}


prim crap() {
}


void cpp_extrasLoad() {
    atl_primdef( cpp_extras );
}

