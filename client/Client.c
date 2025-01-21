
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../includes/utils.h"
#include "../includes/logger.h"
#include <errno.h>

#define LOCAL_SERVER_PORT "8090"
#define REMOTE_HOST "167.71.189.187"
#define REMOTE_PORT "8080"

int localServerSock = -1;
int clientSock = -1;
int serverSock = -1;

void cleanup() {
    puts("\nCleaning up held resources!\n");
    if (localServerSock >= 0) {
        close(localServerSock);
    }
    if (clientSock >= 0) {
        close(clientSock);
    }
    exit(EXIT_FAILURE);
}

void cleanup_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        cleanup();
    }
}

int main(int argc, char *argv[]) {
    localServerSock = CreateServerSocket(LOCAL_SERVER_PORT);

    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid crashes on client disconnection

    if (localServerSock < 0) {
        LogSystemError("Failed to create local server socket");
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    fd_set read_set;
    uint8_t dataBuf[STREAM_BUF_SIZE], servBuf[STREAM_BUF_SIZE];
    struct timeval interval = {10, 0}; // Timeout for select

    while (1) {

        if (clientSock < 0) {
            clientSock = AcceptTCPConnection(localServerSock);
            set_nonblocking_socket(clientSock);
            if (clientSock < 0 && errno != EINTR) {
                perror("Accept failed");
                continue;
            }
            printf("Client connected.\n");
        }

        if (serverSock < 0) {
            serverSock = CreateClientSocket(REMOTE_HOST, REMOTE_PORT);
            set_nonblocking_socket(serverSock);
            if (serverSock < 0) {
                perror("Failed to connect to server. Retrying...");
                sleep(1);
                continue;
            }
            printf("Connected to server.\n");
        }

        FD_ZERO(&read_set);
        FD_SET(clientSock, &read_set);
        FD_SET(serverSock, &read_set);

        int max_fd = (clientSock > serverSock ? clientSock : serverSock) + 1;
        int activity = select(max_fd, &read_set, NULL, NULL, &interval);

        if (activity > 0) {

            if (FD_ISSET(clientSock, &read_set)) {
                ssize_t bytes = read(clientSock, dataBuf, STREAM_BUF_SIZE - 1);
                //ssize_t bytes = FrameFromSocket(dataBuf, STREAM_BUF_SIZE, clientSock);

                if (bytes > 0) {
                    printf("Received from client: %.*s\n", (int)bytes, dataBuf);
                    size_t writeBytes = write(serverSock, dataBuf, bytes);
                    //size_t writeBytes = FrameToSocket(serverSock, dataBuf, sizeof(dataBuf));
                    if (writeBytes > 0) {
                        printf("Successfully wrote %zu bytes to server", writeBytes);
                    } else {
                        printf("Unknown error!");
                    }

                } else {
                    printf("Client disconnected.\n");
                    close(clientSock);
                    clientSock = -1;
                }
            }

            if (FD_ISSET(serverSock, &read_set)) {
                ssize_t bytes = read(serverSock, servBuf, STREAM_BUF_SIZE - 1);
                //ssize_t bytes = FrameFromSocket(servBuf, STREAM_BUF_SIZE, serverSock);
                if (bytes > 0) {
                    printf("Received from server: %.*s\n", (int)bytes, servBuf);
                    //FrameToSocket(clientSock, servBuf, sizeof(servBuf));
                    write(clientSock, servBuf, sizeof(servBuf));
                } else {
                    printf("Server disconnected.\n");
                }
                close(serverSock);
                serverSock = -1;
                printf("Server reconnected!");
                continue;
            }
        } else if (activity == 0) {
            continue;
        } else if (errno != EINTR) {
            perror("Select error");
            break;
        }

    }

    cleanup();
    return 0;
}
