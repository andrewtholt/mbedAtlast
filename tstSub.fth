
0 memsafe

variable msg

: sub-test
    DBASE "LED1" "??" add-record
    mkmsg msg !

    msg-parser msg @ iam "LED1" subscribe 0= if
        msg @ 72 dump cr
        msg @ led@ send-msg
        
        0 msg !
    then
;

sub-test


