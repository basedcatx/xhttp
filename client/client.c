#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include "../includes/utils.h"
#include "../includes/logger.h"

#define LOCAL "0.0.0.0"
#define SERVER_PORT "8080"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to avoid crashing on client disconnection

    int localServerSock = CreateServerSocket(SERVER_PORT);

    if (localServerSock < 0) {
        LogSystemError("localserver socket()");
    }

    while (1) {
        // Accept a client connection
        int clientSock = AcceptTCPConnection(localServerSock);

        if (clientSock < 0) {
            continue; // Skip to the next iteration if accept() fails
        }

        // Wrap the socket FD in a FILE* stream
        FILE *in = fdopen(clientSock, "r+");

        if (in == NULL) {
            perror("fdopen()");
            close(clientSock);
            continue;
        }

        // Buffer to store incoming data
        uint8_t buffer[BUFFER_SIZE];

        while (1) {
            size_t bytesRead = fread(buffer, sizeof(uint8_t), BUFFER_SIZE, in);

            if (bytesRead > 0) {
                // Null-terminate the buffer and print the received data
                buffer[bytesRead] = '\0';
                printf("Received data: %s\n", buffer);
            } else if (feof(in)) {
                // End of file (client disconnected)
                printf("Client disconnected.\n");
                close(clientSock);

                break;
            } else if (ferror(in)) {
                // Error occurred while reading
                perror("fread()");
                break;
            }

        }

        // Clean up resources
        fclose(in);
        close(clientSock);
        printf("Connection closed.\n");
    }
}



