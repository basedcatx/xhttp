//
// Created by BaseDCaTx on 1/16/2025.
//

#ifndef XHTTP_PACKET_H
#define XHTTP_PACKET_H

#include <netinet/in.h>
#include <string.h>
#include <malloc.h>
#include "utils.h"
#include <time.h>

enum {
    COMPRESSION_FLAG = 0x0100,
    CONTINUATION_FLAG = 0x0200,
    IS_RESPONSE_FLAG = 0x0300,
    IS_CHUNK_FLAG = 0x0400,
    IS_REQUEST_FLAG = 0x0500,
};


#include <stdint.h>

#define HTTP_HEADER_TEMPLATE "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n"
#define MIN_CONTENT_LENGTH 100
#define MAX_CONTENT_LENGTH 999


struct Packet {
    uint32_t msgLength;
    uint32_t structSize;
    uint8_t flag;
    uint8_t message[2048];
};

uint8_t *BufferEncode(struct Packet *pck, size_t bufSize, int *bytesWritten);
int BufferDecode(uint8_t *buffer, size_t bufSize, struct Packet *pck);
//int FrameFromSocket (uint8_t *buf, size_t bufSize, int fd);
//int FrameToSocket (int sock, uint8_t *buf, size_t buf_len);
ssize_t FrameToSocket(int sock, const char *header, const void *data, size_t data_size);
ssize_t FrameFromSocket(int sock, const char *header, void *data, size_t data_size);

#endif //XHTTP_PACKET_H
