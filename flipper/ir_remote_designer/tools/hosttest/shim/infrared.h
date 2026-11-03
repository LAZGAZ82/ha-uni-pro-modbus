#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef enum { InfraredProtocolUnknown = -1, InfraredProtocolNEC = 0, InfraredProtocolMAX } InfraredProtocol;
typedef struct {
    InfraredProtocol protocol;
    uint32_t address;
    uint32_t command;
    bool repeat;
} InfraredMessage;
