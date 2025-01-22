//
// Created by BaseDCaTx on 1/16/2025.
//

#include "../includes/packet.h"
#include <netinet/in.h>
#include "../includes/logger.h"
#include <string.h>
#include <malloc.h>
#include "../includes/crypt.h"
#define HEADER_SIZE 40
#define LOG(fmt, ...) fprintf(stderr, "[LOG] " fmt "\n", ##__VA_ARGS__)
#include "errno.h"

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

ssize_t FrameToSocket(int sock, const char *header, const void *data, size_t data_size) {
    size_t total_size = HEADER_SIZE + data_size;
    char *buffer = malloc(total_size);
    if (!buffer) {
        perror("Malloc failed for FrameToSocket");
        return -1;
    }

    // Prepend the header
    memcpy(buffer, header, HEADER_SIZE);
    memcpy(buffer + HEADER_SIZE, data, data_size);

    ssize_t offset = 0;
    while (offset < total_size) {
        ssize_t bytes_written = write(sock, buffer + offset, total_size - offset);
        if (bytes_written <= 0) {
            perror("Socket write failed");
            free(buffer);
            return -1;
        }
        LOG("Sent %zd bytes (offset: %zu)", bytes_written, offset);
        offset += bytes_written;

        // If partial write, prepend the header again
        if (offset < total_size) {
            LOG("Partial write detected. Re-prepending header for remaining data.");
            size_t remaining_size = total_size - offset;
            char *temp_buffer = malloc(HEADER_SIZE + remaining_size);
            if (!temp_buffer) {
                perror("Malloc failed for partial write");
                free(buffer);
                return -1;
            }
            memcpy(temp_buffer, header, HEADER_SIZE);
            memcpy(temp_buffer + HEADER_SIZE, buffer + offset, remaining_size);
            free(buffer);
            buffer = temp_buffer;
            total_size = HEADER_SIZE + remaining_size;
            offset = 0;
        }
    }

    free(buffer);
    LOG("FrameToSocket completed successfully.");
    return offset;
}

ssize_t FrameFromSocket(int sock, const char *header, void *data, size_t data_size) {
    if (sock < 0 || !header || !data) {
        fprintf(stderr, "Invalid input arguments\n");
        return -1;
    }

    char *buffer = malloc(HEADER_SIZE + data_size);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        return -1;
    }

    char *output = malloc(data_size);
    if (!output) {
        perror("Failed to allocate memory for output buffer");
        free(buffer);
        return -1;
    }

    size_t offset = 0, output_offset = 0;

    ssize_t total_bytes_read = 0;


    while (total_bytes_read < data_size) {

        ssize_t bytes_read = read(sock, buffer + offset, HEADER_SIZE + data_size - offset);

        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // No data available, try again
            } else {
                perror("Socket read failed");
                free(buffer);
                free(output);
                return -1;
            }
        }

        offset += bytes_read;
        total_bytes_read += bytes_read;
        LOG("Read %zd bytes (offset: %zu)", bytes_read, offset);

        size_t i = 0;
        while (i + HEADER_SIZE <= offset) {  // Ensure we don't overflow during memcmp
            if (memcmp(buffer + i, header, HEADER_SIZE) == 0) {
                LOG("Header detected at offset %zu. Skipping %d bytes.", i, HEADER_SIZE);
                i += HEADER_SIZE;
            } else {
                if (output_offset < data_size) {
                    output[output_offset++] = buffer[i++];
                    total_bytes_read++;
                } else {
                    i++;
                }
            }
            printf("Iterating\n");
        }


        size_t unprocessed_bytes = offset - i;
        memmove(buffer, buffer + i, unprocessed_bytes);  // Move unprocessed data to the start
        offset = unprocessed_bytes;
    }

    memcpy(data, output, data_size);
    free(buffer);
    free(output);

    LOG("FrameFromSocket completed successfully. Total payload read: %zd bytes.", total_bytes_read);
    return total_bytes_read;
}


