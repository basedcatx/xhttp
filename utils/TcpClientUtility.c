//
// Created by BaseDCaTx on 1/16/2025.
//

#include <string.h>
#include <netdb.h>
#include "../includes/logger.h"


int CreateClientSocket(const char *host, const char *service) {
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(host, service, &addrCriteria, &servAddr);

    if (rtnVal < 0) {
        LogSystemError("getaddrinfo() failed");
    }

    int sock = -1;
    for (struct addrinfo *addr = servAddr; addr->ai_next != NULL; addr = addr->ai_next) {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock < 0) {
            continue;
        }
        break;
    }

    freeaddrinfo(&addrCriteria);
    return sock;
}