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
    
    led@ out @ send-msg

    0 msg !
;

: led-off
    mkmsg out !
    iam out @ set-sender
    hi-level out @ set-type
    SET out @ set-op
    
    "LED1" out @ set-topic
    "OFF" out @ set-msg
    
    out @ msg-dump
    
    led@ out @ send-msg

    0 msg !
;
    
