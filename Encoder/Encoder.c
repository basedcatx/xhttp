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
        return -1;
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int FrameToSocket(int sock, uint8_t *buf, size_t buf_len) {
    if (sock < 0) {
        return -1;
    }

    char http_header[45];
    generate_http_header((char *) &http_header, 40);
    // Allocate memory dynamically
    size_t header_len = strlen(http_header);

    size_t nBuf_size = buf_len + header_len;

    if (nBuf_size > STREAM_BUF_SIZE + 40 || buf_len < 1) {
        fprintf(stderr, "Buffer size too large|small\n");
        return -1;
    }

    printf("%zu\n", nBuf_size);

    uint8_t *nBuf = malloc(nBuf_size);
    memset(nBuf, 0, nBuf_size);

    if (!nBuf) {
        perror("Memory allocation failed");
        return -1;
    }

    size_t offset = 0;

    // Copy header
    strcpy((char *)nBuf + offset, http_header);
    offset += header_len;

    // Copy buffer data
    memcpy(nBuf + offset, buf, buf_len);

    // Write to socket
    ssize_t bytes_written = write(sock, nBuf, nBuf_size);

    free(nBuf);  // Free allocated memory

    if (bytes_written >= 0) {
        return bytes_written;
    } else {
        perror("Socket write failed");
        return -1;
    }
}

int FrameFromSocket(uint8_t *buf, size_t bufSize, int fd) {
    if (fd < 0) {
        return -1;
    }

    size_t header_len = 40;

    if (bufSize < header_len) {
        fprintf(stderr, "Buffer size too small for header\n");
        return -1;
    }

    // Allocate memory dynamically

    size_t tempBuf_size = bufSize + header_len;

    if (tempBuf_size > STREAM_BUF_SIZE + 40) {
        fprintf(stderr, "Buffer size too large\n");
        return -1;
    }

    uint8_t *tempBuf = malloc(tempBuf_size);
    memset(tempBuf, 0, tempBuf_size);

    if (!tempBuf) {
        perror("Memory allocation failed");
        return -1;
    }

    // Read from socket
    ssize_t bytes_read = read(fd, tempBuf, tempBuf_size);

    if (bytes_read < 0) {
        perror("Socket read failed");
        free(tempBuf);
        return -1;
    }

    // Skip the header and copy the rest
    memcpy(buf, tempBuf + header_len, bufSize);

    free(tempBuf);  // Free allocated memory

    return bytes_read - header_len;
}
