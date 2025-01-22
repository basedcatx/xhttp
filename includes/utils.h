//
// Created by BaseDCaTx on 1/16/2025.
//

#ifndef XHTTP_UTILS_H
#define XHTTP_UTILS_H
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "packet.h"

#define STREAM_BUF_SIZE BUFSIZ

void set_nonblocking_socket(int sock);
int CreateServerSocket(const char *service);
int CreateClientSocket(const char *host, const char *service);
uint8_t *zlib_compress_dynamic(const uint8_t *inBuf, size_t bufSize, size_t *outCompressedSize);
uint8_t *zlib_decompress_dynamic(const uint8_t *inBuf, size_t compressedSize, size_t *outDecompressedSize);
void printSocketAddress(const struct sockaddr *address, FILE *stream);
int AcceptTCPConnection(int serveSocks);
void generate_http_header(char *header, size_t header_size);


#endif //XHTTP_UTILS_H
