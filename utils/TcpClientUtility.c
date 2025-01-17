#include <string.h>
#include <netdb.h>
#include "../includes/logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>  // For setting socket options

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

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock < 0) {
            perror("socket()");
            continue;
        }

        // Set the socket to non-blocking mode

        // Attempt to connect (non-blocking)
        int connectResult = connect(sock, addr->ai_addr, addr->ai_addrlen);

        if (connectResult < 0 && errno != EINPROGRESS) {
            // Immediate error handling
            LogSystemError("connect()");
            close(sock);
            sock = -1;
            continue;
        }
        // If connection is established or there's an error, we can exit the loop
        break;
    }

    freeaddrinfo(servAddr);
    return sock;
}
