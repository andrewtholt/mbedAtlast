
0 memsafe

forget msg
variable msg

: get-count
    mkmsg msg !

    msg-parser msg @ iam "COUNT" get-msg
    msg @ msg-dump

    led-tid msg @ send-msg

    0 msg !
;


: get-led
    mkmsg msg !

    msg-parser msg @ iam "LED1" get-msg
    msg @ msg-dump

    led-tid msg @ send-msg

    0 msg !
;



