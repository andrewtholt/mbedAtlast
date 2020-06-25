set pagination off
target remote localhost:3333
monitor reset halt
load
monitor reset halt
br main
c


