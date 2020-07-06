forget out

variable out

: led-on
    mkmsg out !
    iam out @ set-sender
    hi-level out @ set-type
    SET out @ set-op

    "LED1" out @ set-topic
    "ON" out @ set-msg

    out @ msg-dump

    led-tid out @ send-msg

    0 out !
;

: led-off
    mkmsg out !
    iam out @ set-sender
    hi-level out @ set-type
    SET out @ set-op

    "LED1" out @ set-topic
    "OFF" out @ set-msg

    out @ msg-dump

    led-tid out @ send-msg

    0 out !
;

: led-get
    mkmsg out !
    iam out @ set-sender
    hi-level out @ set-type
    GET out @ set-op

    "LED1" out @ set-topic

    out @ msg-dump

    led-tid out @ send-msg
    0 out !
;


