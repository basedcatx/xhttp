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

uint8_t *BufferEncode(struct Packet *pck, size_t bufSize, int *bytesWritten) {

    if (bufSize < 24 + pck->msgLength) { // Minimum size: 4 + 4 + 1 + message
        *bytesWritten = -1;
        return NULL;
    }

    uint8_t *buffer = (uint8_t *) malloc(sizeof(uint8_t) * bufSize);

    if (buffer == NULL) {
        LogErrorWithReason("malloc failed", "buffer is null");
        return NULL;
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

    size_t compressed_size = 0;


    uint8_t *temporal_buffer = zlib_compress_dynamic(buffer, bufSize, (size_t *) &compressed_size);
    if (temporal_buffer != NULL) {
        printf("Hello!");
    }

    if (compressed_size > 0 && temporal_buffer != NULL) {
        uint8_t *newBuf = realloc(buffer, compressed_size);
        aes_gcm_encrypt(temporal_buffer, compressed_size, newBuf);
        free(temporal_buffer);
        *bytesWritten = compressed_size;
        if (newBuf != NULL) {
            return newBuf;
        } else {
            LogErrorWithReasonX("Encoder", "Buffer is null");
            return NULL;
        }
    } else {
        LogErrorWithReasonX("Compression", "An error occurred!");
    }

    *bytesWritten = compressed_size;
    return NULL;
}


int BufferDecode(uint8_t *buffer, const size_t bufSize, struct Packet *pck) {

    if (bufSize < 9) { // Minimum size for header (4 + 4 + 1)
        return -1; // Buffer too small
    }

    uint8_t temporal_buffer[bufSize];

    aes_gcm_decrypt(buffer, bufSize, temporal_buffer);
    size_t outDecompressedSize = 0;
    buffer = zlib_decompress_dynamic(temporal_buffer, bufSize, &outDecompressedSize);

    if (outDecompressedSize == 0) {
        LogErrorWithReason("decompression", "ZLIB Decompression potential issues!");
    }

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


    // Copy message payload
     memcpy(pck->message, buffer + offset, pck->msgLength);
     offset += pck->msgLength;

    free(buffer);

    return offset; // Return total bytes read
}