
0 memsafe

variable msg

: sub-test
    DBASE "LED1" "??" add-record
    mkmsg msg !

    msg-parser msg @ iam "COUNT" subscribe-msg 0= if
        msg @ msg-dump
        led@ msg @ send-msg

        0 msg !
    then
;

sub-test

