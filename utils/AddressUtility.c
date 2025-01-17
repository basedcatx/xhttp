//
// Created by BaseDCaTx on 1/16/2025.
//

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void printSocketAddress(const struct sockaddr *address, FILE *stream) {
    if (address == NULL || stream == NULL)
        return;
    void *numericAddress;
    char addrBuff[INET6_ADDRSTRLEN];
    in_port_t port;

    switch (address->sa_family) {
        case AF_INET:
            numericAddress = &((struct sockaddr_in *) address)->sin_addr;
            port = ntohs(((struct sockaddr_in *) address)->sin_port);
            break;
        case AF_INET6:
            numericAddress = &((struct sockaddr_in6 *) address)->sin6_addr;
            port = ntohs(((struct sockaddr_in6 *) address)->sin6_port);
            break;
        default:
            fputs("[unknown type]\n", stream); // Unhandled type
            return;
    }

    if (inet_ntop(address->sa_family, numericAddress, addrBuff, sizeof(addrBuff)) == NULL)
        fputs("[Invalid address]", stream); // Unable to convert!
    else {
        fprintf(stream, "%s", addrBuff);
        if (port != 0) {
            fprintf(stream, "\nport: %u", port);
        }
    }
}
