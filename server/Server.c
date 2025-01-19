#include "../includes/utils.h"
#include "../includes/logger.h"
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEF_LOCAL_PORT "8081"
#define PROXY_HOST "localhost"
#define PROXY_PORT "3128"
#define BUFFER_SIZE 1024

int serverSock = -1;
int proxySocks = -1;
int clientSock = -1;
FILE *clientStream;
FILE *proxyStream;

void cleanup() {
    puts("\nCleaning up held resources!\n");
    if (serverSock >= 0) {
        close(serverSock);
    }
    if (proxySocks >= 0) {
        close(proxySocks);
    }
    exit(EXIT_FAILURE);
}

void cleanup_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        cleanup();
    }
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid crashing on client disconnection

    serverSock = CreateServerSocket(DEF_LOCAL_PORT);

    proxySocks = CreateClientSocket(PROXY_HOST, PROXY_PORT);


    if (serverSock < 0) {
        LogSystemError("serverSocks()");
        exit(EXIT_FAILURE);
    }

    proxyStream = fdopen(proxySocks, "r+");
    setvbuf(proxyStream, NULL, _IONBF, 0);

    if (proxySocks < 0) {
        LogSystemError("proxySocks()");
        exit(EXIT_FAILURE);
    }

    if (proxyStream == NULL) {
        perror("proxy");
        exit(EXIT_FAILURE);
    }


    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0 || sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    fd_set read_fds, write_fds;
    int max_fd = (serverSock > proxySocks ? serverSock : proxySocks) + 1;

    char buffer[BUFFER_SIZE];

    while (1) {

        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(serverSock, &read_fds);  // Monitor server socket for new connections
        FD_SET(proxySocks, &read_fds); // Monitor proxy socket

        int activity = select(max_fd, &read_fds, &write_fds, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (FD_ISSET(serverSock, &read_fds)) {
            // Accept new client connection
            clientSock = AcceptTCPConnection(serverSock);

            if (clientSock < 0) {
                perror("AcceptTCPConnection");
                continue;
            }


            printf("New client connected: %d\n", clientSock);

            // Add client socket to the set
            FD_SET(clientSock, &read_fds);

            if (clientSock >= max_fd) {
                max_fd = clientSock + 1;
            }

            clientStream = fdopen(clientSock, "r+");

            if (clientStream == NULL) {
                perror("c_stream");
                continue;
            }
        }


        for (int i = 0; i < max_fd; ++i) {
            if (FD_ISSET(i, &read_fds) && i != serverSock && i != proxySocks) {

                ssize_t bytesRead = fread(buffer, 1, BUFFER_SIZE, clientStream);

                if (bytesRead > 0) {
                    printf("Received from client %d: %.*s", i, (int)bytesRead, buffer);
                    // Forward data to proxy
                    if (fwrite(buffer, 1, bytesRead, proxyStream) < 0) {
                        perror("write to proxy");
                    }

                    fflush(proxyStream);
                } else if (bytesRead == 0) {
                    printf("Client %d disconnected.\n", i);
                    close(i);
                    FD_CLR(i, &read_fds);
                } else {
                    perror("read from client");
                    close(i);
                    FD_CLR(i, &read_fds);
                }
            }
        }

        if (FD_ISSET(proxySocks, &read_fds)) {

            // Read from proxy socket
            ssize_t bytesRead = fread( buffer,1, BUFFER_SIZE, proxyStream);

            if (bytesRead > 0) {
                printf("Received from proxy: %.*s", (int)bytesRead, buffer);
                ssize_t byteWritten = fwrite(buffer, 1, BUFFER_SIZE, clientStream);
                fflush(clientStream);

                if (byteWritten > 0) {
                    puts("Successfully written to client");
                }
                // You can forward this data to a client socket here
            } else if (bytesRead == 0) {
                continue;
            } else {
                perror("read");
                break;
            }
        }
    }

    cleanup();
    return 0;
}
