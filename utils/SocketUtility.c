//
// Created by BaseDCaTx on 1/16/2025.
//
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

void set_nonblocking_socket(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}
