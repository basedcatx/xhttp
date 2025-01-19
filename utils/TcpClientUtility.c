#include <string.h>
#include <netdb.h>
#include "../includes/logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>  // For setting socket options
#include <netinet/tcp.h>
#include "../includes/utils.h"
int CreateClientSocket(const char *host, const char *service) {
    struct addrinfo addrCriteria;
    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;       // IPv4 or IPv6
    addrCriteria.ai_socktype = SOCK_STREAM;  // Stream socket
    addrCriteria.ai_protocol = IPPROTO_TCP;  // TCP protocol

    struct addrinfo *servAddr = NULL;
    int rtnVal = getaddrinfo(host, service, &addrCriteria, &servAddr);

    if (rtnVal != 0) {  // Check for errors
        LogSystemError(gai_strerror(rtnVal)); // Log human-readable error
        return -1;
    }

    int sock = -1;  // Initialize socket descriptor
    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (sock < 0) {
            perror("socket() failed");
            continue;  // Try next address
        }

        size_t tBuf = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &tBuf, sizeof(tBuf)) < 0) {
            perror("setsockopt(SO_REUSEPORT) failed");
            close(sock);
            sock = -1;
            continue;
        }

        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &tBuf, sizeof(tBuf)) < 0) {
            perror("setsockopt(TCP_NODELAY) failed");
            close(sock);
            sock = -1;
            continue;
        }

        set_nonblocking_socket(sock);

        if (connect(sock, addr->ai_addr, addr->ai_addrlen) < 0 && errno != EINPROGRESS) {
            LogSystemError("connect() failed");
            close(sock);
            sock = -1;
            continue;
        }

        // If connect succeeded or is in progress, break the loop
        break;
    }

    freeaddrinfo(servAddr);  // Always free addrinfo memory

    if (sock < 0) {
        LogSystemError("Failed to create and connect socket");
    }

    return sock;
}
