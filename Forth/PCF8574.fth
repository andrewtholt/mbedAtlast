forget i2c

variable i2c

2 string cmd

: i2c-write-test
    
    cmd c!

    i2c-open i2c !

    cmd 1 0x40 i2c @ i2c-write

    i2c @ i2c-close
;

: i2c-read-test
    i2c-open i2c !
    cmd 1 0x40 i2c @ i2c-read
    i2c @ i2c-close
;
\ 
\ Write 1 to a bit position if you want to use it as an input.  
\ e.g. All i/p
\ 
\ 0xff i2c-write-test
\ 
\ i2c-read-test cmd 2 dump
\ 
