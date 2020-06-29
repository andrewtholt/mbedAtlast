forget i2c

variable i2c
variable init-ran
0 init-ran !

2 string cmd

: i2c-init

    init-ran @ 0= if
        i2c-open i2c !
        -1 init-ran !
    then
;

: i2c-write-test
    i2c-init 
    cmd c!

    i2c-open i2c !

    cmd 1 0x40 i2c @ i2c-write

    i2c @ i2c-close
;

: i2c-read-test
    i2c-init 
    i2c-open i2c !
    cmd 1 0x40 i2c @ i2c-read
    i2c @ i2c-close
;

: blink-test
    i2c-init 

    0 do 
        0xaa i2c-write-test
        500 ms
        0x55 i2c-write-test
        500 ms
    loop
;

9 string bar-value

0x00 bar-value c!
0x01 bar-value 1+ c!
0x03 bar-value 2+ c!
0x07 bar-value 3 + c!
0x0f bar-value 4 + c!
0x1f bar-value 5 + c!
0x3f bar-value 6 + c!
0x7f bar-value 7 + c!
0xff bar-value 8 + c!

: bar \ value -- bar
    bar-value + c@
;

: bar-set \ vale --
    bar i2c-write-test 
;

: bar-test
    i2c-init 

    9 0 do 
        i . cr
        i bar-set

        200 ms
    loop
;

\ 
\ Write 1 to a bit position if you want to use it as an input.  
\ e.g. All i/p
\ 
\ 0xff i2c-write-test
\ 
\ i2c-read-test cmd 2 dump
\ 
