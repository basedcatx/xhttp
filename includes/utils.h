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

void set_nonblocking_socket(int sock);
int CreateServerSocket(const char *service);
int CreateClientSocket(const char *host, const char *service);
int zlib_compress (const uint8_t *inBuf, uint8_t *outBuf, size_t bufSize);
int zlib_decompress(const uint8_t *input, uint8_t *output, size_t bufferSize);
void printSocketAddress(const struct sockaddr *address, FILE *stream);
int AcceptTCPConnection(int serveSocks);

#endif //XHTTP_UTILS_H
