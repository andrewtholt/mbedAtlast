#!/usr/bin/env expect
#

spawn /usr/bin/telnet localhost 4444
expect "> "

send "reset halt\n"
expect "> "

send "program ./BUILD/NUCLEO_F411RE/GCC_ARM/mbed.elf\n"
expect "> "

send "exit\n"

