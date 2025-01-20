//
// Created by BaseDCaTx on 1/16/2025.
//

#include <string.h>
#include <netdb.h>
#include "../includes/logger.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include "../includes/utils.h"
#define MAX_CONNECTED_SOCKS 10

int CreateServerSocket(const char *service) {
    struct addrinfo addrCriteria;

    memset(&addrCriteria, 0, sizeof(addrCriteria));
    addrCriteria.ai_family = AF_UNSPEC;
    addrCriteria.ai_flags = AI_PASSIVE;
    addrCriteria.ai_socktype = SOCK_STREAM;
    addrCriteria.ai_protocol = IPPROTO_TCP;

    struct addrinfo *servAddr;
    int rtnVal = getaddrinfo(NULL, service, &addrCriteria, &servAddr);

    if (rtnVal != 0) {
        LogErrorWithReasonX("get-addr-info () failed", gai_strerror(rtnVal));
    }

    int serverSock = -1;

    for (struct addrinfo *addr = servAddr; addr != NULL; addr = addr->ai_next) {
        serverSock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        set_nonblocking_socket(serverSock);
        if (serverSock < 0) {
            continue;
        }

        if (bind(serverSock, servAddr->ai_addr, servAddr->ai_addrlen) == 0 && (listen(serverSock, MAX_CONNECTED_SOCKS) == 0)) {
            struct sockaddr_storage localAddr;
            socklen_t addrSize = sizeof(localAddr);

            if (getsockname(serverSock, (struct sockaddr *) &localAddr, &addrSize) < 0) {
                LogSystemError("getsockname falied!");
            }

            fputs("Binding to ", stdout);
            printSocketAddress((struct sockaddr *) &localAddr, stdout);
            fputc('\n', stdout);
            freeaddrinfo(servAddr);
            break;
        }
        close(serverSock);
        serverSock = -1;
    }
    return serverSock;
}

int AcceptTCPConnection(int serveSocks) {
    struct sockaddr_storage clntAddr;
    socklen_t clntAddrLen = sizeof(clntAddr);
    fd_set sock_fd_set;
    FD_ZERO(&sock_fd_set);
    FD_SET(serveSocks, &sock_fd_set);

    size_t tBuf = 1;
    setsockopt(serveSocks, SOL_SOCKET, SO_REUSEPORT, &tBuf, sizeof(tBuf));

    while (1) {
        struct timeval timeout = {5, 0};
        int ready = select(serveSocks + 1, &sock_fd_set, NULL, NULL, &timeout);

        if (ready == -1) {
            LogSystemError("select() failed!");
            break;
        }

        if (ready == 0) {
            LogErrorWithReason("select() timed out. No incoming connection.", "");
            continue;
        }

        if (FD_ISSET(serveSocks, &sock_fd_set)) {
            int clntSock = accept(serveSocks, (struct sockaddr *)&clntAddr, &clntAddrLen);
            if (clntSock < 0) {
                LogSystemError("accept() failed!");
                continue;
            }

            fputs("Handling client ", stdout);
            printSocketAddress((const struct sockaddr *)&clntAddr, stdout);
            fputc('\n', stdout);
            return clntSock;
        }
    }
    return -1; // In case of persistent failure
}


