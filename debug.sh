#!/bin/bash 

set -x

TARGET=./BUILD/ARCH_MAX/GCC_ARM/mbed.elf

gdb-multiarch -x dbg.gdb  $TARGET
