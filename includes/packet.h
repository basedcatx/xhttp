//
// Created by BaseDCaTx on 1/16/2025.
//

#ifndef XHTTP_PACKET_H
#define XHTTP_PACKET_H

#include <netinet/in.h>
#include <string.h>
#include <malloc.h>
#include "utils.h"

enum {
    COMPRESSION_FLAG = 0x0100,
    CONTINUATION_FLAG = 0x0200,
    IS_RESPONSE_FLAG = 0x0300,
    IS_CHUNK_FLAG = 0x0400
};


#include <stdint.h>

static const char *HTTP_HEADER_STR = "HTTP/1.1 200 OK\r\nContent-Length: 148\r\n";

struct Packet {
    uint32_t msgLength;
    uint32_t structSize;
    uint8_t flag;
    uint8_t message[STREAM_BUF_SIZE];
};

uint8_t *BufferEncode(struct Packet *pck, size_t bufSize, int *bytesWritten);
int BufferDecode(uint8_t *buffer, size_t bufSize, struct Packet *pck);

#endif //XHTTP_PACKET_H
