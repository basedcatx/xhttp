//
// Created by BaseDCaTx on 1/16/2025.
//
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include "../includes/logger.h"

// Note input buffer must be same size as output buffer
// Compress data, dynamically allocating enough space if needed

uint8_t *zlib_compress_dynamic(const uint8_t *inBuf, size_t bufSize, size_t *outCompressedSize) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        fprintf(stderr, "Failed to initialize compression\n");
        return NULL;
    }

    size_t outBufSize = bufSize; // Start with the input size
    uint8_t *outBuf = malloc(outBufSize);

    if (!outBuf) {
        LogErrorWithReason("ZLIB", "Memory allocation failed\n");
        deflateEnd(&stream);
        return NULL;
    }

    while (1) {
        stream.next_in = (uint8_t *)inBuf;
        stream.avail_in = bufSize;
        stream.next_out = outBuf;
        stream.avail_out = outBufSize;

        int ret = deflate(&stream, Z_FINISH);
        if (ret == Z_STREAM_END) {
            break; // Compression succeeded
        } else if (ret == Z_OK || ret == Z_BUF_ERROR) {
            // Increase buffer size and try again
            outBufSize *= 2;
            uint8_t *newBuf = realloc(outBuf, outBufSize);

            if (!newBuf) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(outBuf);
                deflateEnd(&stream);
                return NULL;
            }
            outBuf = newBuf;
        } else {
            fprintf(stderr, "Compression failed\n");
            free(outBuf);
            deflateEnd(&stream);
            return NULL;
        }
    }

    *outCompressedSize = outBufSize;
    deflateEnd(&stream);

    return outBuf;
}

// Decompress data, dynamically allocating enough space if needed
uint8_t *zlib_decompress_dynamic(const uint8_t *inBuf, size_t compressedSize, size_t *outDecompressedSize) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        LogErrorWithReasonX("ZLIB", "Failed to initialize decompression\n");
        return NULL;
    }

    size_t outBufSize = compressedSize * 2; // Start with twice the compressed size
    uint8_t *outBuf = malloc(outBufSize);
    if (!outBuf) {
        fprintf(stderr, "Memory allocation failed\n");
        inflateEnd(&stream);
        return NULL;
    }

    while (1) {
        stream.next_in = (uint8_t *)inBuf;
        stream.avail_in = compressedSize;
        stream.next_out = outBuf;
        stream.avail_out = outBufSize;

        int ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_END) {
            break; // Decompression succeeded
        } else if (ret == Z_OK || ret == Z_BUF_ERROR) {
            // Increase buffer size and try again
            outBufSize *= 2;
            uint8_t *newBuf = realloc(outBuf, outBufSize);
            if (!newBuf) {
                fprintf(stderr, "Memory reallocation failed\n");
                free(outBuf);
                inflateEnd(&stream);
                return NULL;
            }
            outBuf = newBuf;
        } else {
            fprintf(stderr, "Decompression failed\n");
            free(outBuf);
            inflateEnd(&stream);
            return NULL;
        }
    }

    *outDecompressedSize = stream.total_out;
    inflateEnd(&stream);

    return outBuf;
}
