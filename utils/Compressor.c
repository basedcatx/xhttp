//
// Created by BaseDCaTx on 1/16/2025.
//
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>

// Note input buffer must be same size as output buffer

int zlib_compress (const uint8_t *inBuf, uint8_t *outBuf, size_t bufSize) {
    memset(outBuf, 0, sizeof(bufSize));

    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    // Initialize deflate for Gzip format (15 + 16)
    if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        fprintf(stderr, "Failed to initialize compression\n");
        exit(EXIT_FAILURE);
    }

    stream.next_in = (uint8_t *)inBuf;
    stream.avail_in = bufSize;
    stream.next_out = outBuf;
    stream.avail_out = bufSize;

    // Compress the data
    if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Compression failed\n");
        deflateEnd(&stream);
        exit(EXIT_FAILURE);
    }

    int total_compressed_bytes = stream.total_out;
    deflateEnd(&stream);
    return  total_compressed_bytes;
}

int zlib_decompress(const uint8_t *input, uint8_t *output, size_t bufferSize) {
    memset(output, 0, sizeof(bufferSize));

    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        fprintf(stderr, "Failed to initialize decompression\n");
        exit(EXIT_FAILURE);
    }

    stream.next_in = (uint8_t *)input;
    stream.avail_in = bufferSize;
    stream.next_out = output;
    stream.avail_out = bufferSize;

    // Decompress the data
    if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
        fprintf(stderr, "Decompression failed\n");
        inflateEnd(&stream);
        exit(EXIT_FAILURE);
    }

    size_t zlib_decompressed_bytes = stream.total_out;
    inflateEnd(&stream);
    return zlib_decompressed_bytes;
}