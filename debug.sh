#!/bin/bash 

set -x

# TARGET=./BUILD/NUCLEO_F411RE/GCC_ARM/mbed.elf
# TARGET=./BUILD/ARCH_MAX/GCC_ARM/mbed.elf

TARGET=./BUILD/DISCO_F407VG/GCC_ARM/mbed.elf

gdb-multiarch -x dbg.gdb  $TARGET
