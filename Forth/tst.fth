
0 memsafe


variable msg

: led-on
    mkmsg msg !
    DBASE "LED1" "ON" add-record
    msg-parser msg @ iam "LED1" db-to-msg 0= if
        msg @ msg-dump cr
        led@ msg @ send-msg
        0 msg !
    then
;


: led-off
    mkmsg msg !
    DBASE "LED1" "OFF" add-record
    msg-parser msg @ iam "LED1" db-to-msg 0= if
        msg @ msg-dump cr
        led@ msg @ send-msg
        0 msg !
    then
;


: i2c-test
    mkmsg msg !
    DBASE "LED1" "OFF" add-record
    msg-parser msg @ iam "LED1" db-to-msg 0= if
        msg @ msg-dump cr
        i2c@ msg @ send-msg
        0 msg !
    then

;
