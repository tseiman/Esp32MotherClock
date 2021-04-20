
#pragma once


#include "websocket.h"


struct downstreamPkt {
    char *buffer; 
    unsigned int len;
} downstreamPkt;
typedef struct downstreamPkt* downstreamPacket_t;


esp_err_t interpreteCmd(char *inputBuffer,size_t inputBufferLen, downstreamPacket_t *downstreamPacket, sessionContext_t sessContext);
void freeDownstreamPacketBuffer(downstreamPacket_t downstreamPacket);