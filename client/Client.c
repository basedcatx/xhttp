#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../includes/utils.h"
#include "../includes/logger.h"
#include "../includes/packet.h"
#include <errno.h>

#define SERVER_PORT "8080"
int localServerSock;
int clientSock;
FILE *in = NULL;

void cleanup() {
    puts("\nCleaning up held resources!\n");
    if (in != NULL) {
        fclose(in);
    }
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

    localServerSock = CreateServerSocket(SERVER_PORT);

    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid crashing on client disconnection

    if (localServerSock < 0) {
        LogSystemError("localserver socket()");
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = cleanup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("signal-action");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        perror("signal-action");
        exit(EXIT_FAILURE);
    }


    while (1) {
        clientSock = AcceptTCPConnection(localServerSock);

        if (clientSock < 0) {
            perror("AcceptTCPConnection");
            continue; // Skip this iteration and wait for a new client
        }

        printf("New client connected...\n");

        uint8_t dataBuffer[STREAM_BUF_SIZE];
        struct timeval waitTime;
        int isReady;

        while (1) {
            // Prepare the readWatcher set
            fd_set readWatcher;
            FD_ZERO(&readWatcher);
            FD_SET(clientSock, &readWatcher);

            // Set timeout for select
            waitTime.tv_sec = 10;
            waitTime.tv_usec = 0;

            isReady = select(clientSock + 1, &readWatcher, NULL, NULL, &waitTime);

            if (isReady > 0) {
                if (FD_ISSET(clientSock, &readWatcher)) {
                    memset(dataBuffer, '\0', sizeof(dataBuffer));
                    ssize_t bytesRead = read(clientSock, dataBuffer, STREAM_BUF_SIZE - 1);

                    if (bytesRead > 0) {
                        // Process the data
                        struct Packet requestPacket;
                        memset(&requestPacket, 0, sizeof(requestPacket));
                        dataBuffer[bytesRead] = '\0';
                        requestPacket.msgLength = strlen((char *)dataBuffer);
                        requestPacket.structSize = sizeof(struct Packet);
                        strncpy((char *)requestPacket.message, (char *)dataBuffer, sizeof(requestPacket.message) - 1);

                        // Encode the packet
                        uint8_t *eBuffer = NULL;
                        int bytesEncoded = -1;
                        eBuffer = BufferEncode(&requestPacket, STREAM_BUF_SIZE, &bytesEncoded);

                        if (bytesEncoded > 0) {
                            printf("Encode complete: %d bytes encoded\n", bytesEncoded);
                        } else {
                            printf("Encoding failed\n");
                        }

                        free(eBuffer);
                    } else if (bytesRead == 0) {
                        // Client disconnected
                        printf("Client disconnected. Waiting for new connections...\n");
                        close(clientSock);
                        break; // Exit the inner loop and wait for a new client
                    } else {
                        // Read error
                        perror("read");
                        break;
                    }
                }
            } else if (isReady == 0) {
                // Timeout
                printf("No data received within the timeout period.\n");
            } else {
                // Error in select
                perror("select");
                close(clientSock);
                break;
            }
        }
    }

}


