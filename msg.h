#pragma once

#define MAX_TOPIC 32
#define MAX_MSG 32
#include "tasks.h"
#include <cstdint>

enum class msgType {
    INVALID=0,
    HI_LEVEL,
    LO_LEVEL
} ;

enum class highLevelOperation {
    INVALID=0,
    NOP,
    GET,
    SET,
    SUB,
    UNSUB,
    EXIT
} ;

typedef struct {
    char topic[MAX_TOPIC];
    char msg[MAX_MSG];
} hl_message_t;

enum class lowLevelOperation {
    NOP=0,
    RD,
    WR,
    IOCTL
} ;

typedef struct {
    uint8_t selector;
    uint16_t deviceRegister;
    void *buffer;
    uint16_t count;
} ll_message_t;

typedef struct {
    taskId Sender;
    msgType type;
    union {
        highLevelOperation hl_op;
        lowLevelOperation ll_op;
    } op ;

    union {
        hl_message_t hl_body;
        ll_message_t ll_body;
    } body;
} message_t;

void mkSubMsg(message_t *msg, taskId sender, char *key) ;
void mkGetMsg(message_t *msg, taskId sender, char *key) ;
void mkSetMsg(message_t *msg, taskId sender, char *key, char *value) ;

void msgToRemote(message_t *msg, uint8_t *remote) ;


