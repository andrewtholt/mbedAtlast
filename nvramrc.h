
// 
// Autogenerated, do not edit.
// 

#ifndef __ATL_SRC
#define __ATL_SRC

uint8_t nvramrc[] = "5 constant LED1\n: sifting\n cr\n token $sift\n;\n: [char] char ; immediate\n: dofield\ndoes>\n dup @\n swap\n cell+ @\n -rot\n + swap\n drop\n;\n: field\n create\n 2dup\n swap\n ,\n ,\n +\n dofield\n;\n0 constant struct\n: endstruct\n create\n ,\n does>\n @\n;\n";

#endif


