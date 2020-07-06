
forget msg

0 memsafe

variable msg

: sub-test
    DBASE "LED1" "??" add-record
    mkmsg msg !

    msg-parser msg @ iam "COUNT" subscribe-msg
    msg @ msg-dump
    led-tid msg @ send-msg

    0 msg !
;

sub-test


