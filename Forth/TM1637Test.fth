
forget cnt

variable cnt
0 cnt !

8 string buffer

buffer 8 0x20 fill

: test
    led-cls
    20 0 do
        0 led-loc
        i "%06ld" buffer strform
        buffer led-type

        500 ms
    loop
;

test

