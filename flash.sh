#!/bin/bash 

TARGET=/BUILD/ARCH_MAX/GCC_ARM/mbed.elf


gdb-multiarch -x cmds.gdb  $TARGET
