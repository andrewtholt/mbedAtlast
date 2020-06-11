#!/bin/bash 

set -x

gdb-multiarch -x dbg.gdb  ./BUILD/NUCLEO_F411RE/GCC_ARM/mbed.elf
