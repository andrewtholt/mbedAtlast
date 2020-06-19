
0 memsafe

variable msg

: get-test
    mkmsg msg !

    msg-parser msg @ iam "COUNT" get-msg
    msg @ msg-dump
    msg @ led@ send-msg

    0 msg !
;

get-test


