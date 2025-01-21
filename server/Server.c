#include "../includes/utils.h"
#include "../includes/logger.h"
#include "../includes/packet.h"
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#define DEF_LOCAL_PORT "8080"
#define PROXY_HOST "localhost"
#define PROXY_PORT "3128"

int serverSock = -1;
int proxySocks = -1;

void cleanup();
void cleanup_handler(int signo);
void *handle_client_thread(void *args);


int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid crashing on client disconnection

    serverSock = CreateServerSocket(DEF_LOCAL_PORT);

    if (serverSock < 0) {
        LogSystemError("serverSocks()");
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

    fd_set read_fds;

    while (1) {

        FD_ZERO(&read_fds);
        FD_SET(serverSock, &read_fds);

        int activity = select(serverSock + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (FD_ISSET(serverSock, &read_fds)) {
            int clientSock = AcceptTCPConnection(serverSock);

            if (clientSock < 0) {
                perror("AcceptTCPConnection");
                continue;
            }

            int *clientSockPtr = malloc(sizeof(int));

            if (!clientSockPtr) {
                perror("malloc");
                close(clientSock);
                continue;
            }

            *clientSockPtr = clientSock;

            pthread_t clientThread;
            if (pthread_create(&clientThread, NULL, handle_client_thread, clientSockPtr) == 0) {
                pthread_detach(clientThread);
                printf("Detached!");
            } else {
                perror("pthread_create");
                close(clientSock);
                free(clientSockPtr);
            }
        }

    }

    cleanup();
    return 0;
}



void *handle_client_thread(void *args) {
    int socks = *(int *)args; // Client socket
    free(args);

    int proxySocket = CreateClientSocket(PROXY_HOST, PROXY_PORT);
    if (proxySocket < 0) {
        perror("Failed to connect to proxy");
        close(socks);
        return NULL;
    }

    // Set both sockets to non-blocking mode
    set_nonblocking_socket(socks);
    set_nonblocking_socket(proxySocket);

    printf("New client connected: %d\n", socks);

    uint8_t client_buf[STREAM_BUF_SIZE];
    uint8_t proxy_buf[STREAM_BUF_SIZE];

    while (1) {
        fd_set read_fd_set, write_fd_set;
        FD_ZERO(&read_fd_set);
        FD_ZERO(&write_fd_set);

        FD_SET(socks, &read_fd_set);
        FD_SET(proxySocket, &read_fd_set);

        int max_fd = (socks > proxySocket ? socks : proxySocket) + 1;

        struct timeval timeout = {5, 0}; // 5-second timeout for select
        int activity = select(max_fd, &read_fd_set, &write_fd_set, NULL, &timeout);

        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (activity == 0) {
            // Timeout
            continue;
        }

        // Handle data from client to proxy
        if (FD_ISSET(socks, &read_fd_set)) {
            size_t client_bytes = read(socks, client_buf, sizeof(client_buf));
            //size_t client_bytes = FrameFromSocket(client_buf, sizeof(client_buf), socks);
            if (client_bytes > 0) {

                ssize_t proxy_sent = write(proxySocket, client_buf, client_bytes);
                //ssize_t proxy_sent = FrameToSocket(proxySocket, client_buf, sizeof(client_buf));
                if (proxy_sent < 0 && errno != EAGAIN) {
                    perror("Error writing to proxy");
                    break;
                }
                printf("Sent %zu bytes to proxy\n", proxy_sent);
            } else if (client_bytes == 0) {
                printf("Client disconnected.\n");
                break;
            } else if (errno != EAGAIN) {
                perror("Error reading from client");
                break;
            }
        }

        // Handle data from proxy to client
        if (FD_ISSET(proxySocket, &read_fd_set)) {
             ssize_t proxy_bytes = read(proxySocket, proxy_buf, sizeof(proxy_buf));
           // ssize_t proxy_bytes = FrameFromSocket(proxy_buf, sizeof(proxy_buf), proxySocket);
            if (proxy_bytes > 0) {
                ssize_t client_sent = write(socks, proxy_buf, proxy_bytes);
                //ssize_t client_sent = FrameToSocket(socks, proxy_buf, sizeof(proxy_buf));
                if (client_sent < 0 && errno != EAGAIN) {
                    perror("Error writing to client");
                    break;
                }
                printf("Sent %zu bytes to client\n", client_sent);
            } else if (proxy_bytes == 0) {
                printf("Proxy disconnected.\n");
                break;
            } else if (errno != EAGAIN) {
                perror("Error reading from proxy");
                break;
            }
        }
    }

    close(socks);
    close(proxySocket);
    printf("Thread exiting for client %d.\n", socks);
    return NULL;
}



void cleanup_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        cleanup();
    }
}

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

