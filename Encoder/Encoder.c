//
// Created by BaseDCaTx on 1/16/2025.
//

#include "../includes/packet.h"
#include <netinet/in.h>
#include "../includes/logger.h"
#include <string.h>
#include <malloc.h>
#include "../includes/utils.h"
#include "../includes/crypt.h"

int BufferEncode(struct Packet *pck, uint8_t *buffer, size_t bufSize) {

    if (bufSize < 24 + pck->msgLength) { // Minimum size: 4 + 4 + 1 + message
        return -1; // Buffer too small
    }

    int offset = 0;

    uint32_t netMsgLength = htonl(pck->msgLength);
    memcpy(buffer + offset, &netMsgLength, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Encode struct size (4 bytes, network byte order)
    uint32_t netStructSize = htonl(pck->structSize);
    memcpy(buffer + offset, &netStructSize, sizeof(uint32_t));
    offset += sizeof(uint32_t);

    // Encode flag (1 byte)
    buffer[offset++] = pck->flag;

    // Copy message payload
    memcpy(buffer + offset, pck->message, pck->msgLength);
    offset += pck->msgLength;

    uint8_t compressedBuf[1024], encryptedBuf[1024];

    uint8_t temporal_buffer[bufSize];
    zlib_compress(buffer,temporal_buffer, bufSize);
    aes_gcm_encrypt(temporal_buffer, bufSize, buffer);

    return offset; // Return total bytes written
}


int BufferDecode(uint8_t *buffer, const size_t bufSize, struct Packet *pck) {
    if (bufSize < 9) { // Minimum size for header (4 + 4 + 1)
        return -1; // Buffer too small
    }

    uint8_t temporal_buffer[bufSize];
    aes_gcm_decrypt(buffer, bufSize, temporal_buffer);
    zlib_decompress(temporal_buffer, buffer , bufSize);

    int offset = 0;

    // Decode message length (4 bytes, network byte order)
    uint32_t netMsgLength;
    memcpy(&netMsgLength, buffer + offset, sizeof(uint32_t));
    pck->msgLength = ntohl(netMsgLength);
    offset += sizeof(uint32_t);

    // Decode struct size (4 bytes, network byte order)
    uint32_t netStructSize;
    memcpy(&netStructSize, buffer + offset, sizeof(uint32_t));
    pck->structSize = ntohl(netStructSize);
    offset += sizeof(uint32_t);

    // Decode flag (1 byte)
    pck->flag = buffer[offset++];


    // Validate buffer size against decoded message length
    if (bufSize < offset + pck->msgLength) {
        return -2; // Buffer too small for message payload
    }

    // Allocate memory for the message payload

    pck->message = (uint8_t *) malloc(pck->msgLength);
    if (pck->message == NULL) {
        return -3; // Memory allocation failed
    }

    // Copy message payload
     memcpy(pck->message, buffer + offset, pck->msgLength);
//    offset += pck->msgLength;

    return offset; // Return total bytes read
}

void FrameToSocket(uint8_t *buf, size_t bufSize, FILE *in) {
    if (fwrite(HTTP_HEADER_STR, sizeof (uint8_t), strlen(HTTP_HEADER_STR), in) < strlen(HTTP_HEADER_STR)) {
        LogErrorWithReason("io", "Failed to send header!");
    }

    fwrite(buf, sizeof(uint8_t), bufSize, in);
}

void FrameFromSocket(uint8_t *outBuf, size_t bufSize, FILE *in) {
    if (fread(NULL, sizeof(uint8_t), strlen(HTTP_HEADER_STR), in) < strlen(HTTP_HEADER_STR)) {
        LogErrorWithReason("io", "Failed to read header!");
    }
    fread(outBuf, sizeof (uint8_t), bufSize, in);
}