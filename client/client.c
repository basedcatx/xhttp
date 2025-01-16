#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "167.71.189.187" 
#define SERVER_PORT 8090
#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <message> <useIncrementals[0/1]> <sleep_timeout[s]> \n", argv[0]);
        exit(EXIT_FAILURE);
    }

        char buffer[BUFFER_SIZE];
        char response[BUFFER_SIZE];


    char* long_message = argv[1];
    int useIncrementals = atoi(argv[2]);
    int sleep_s_timeout = atoi(argv[3]);

    struct sockaddr_in server_addr;

    printf("\n<<Client is starting...>>\n");

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Initialize the payload
    char payload[BUFFER_SIZE];
    snprintf(payload, sizeof(payload), "%s", long_message);

    for (;;) {

        // Create a new socket
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (client_socket < 0) {
            perror("[SOCKET]");
            exit(EXIT_FAILURE);
        }

        // Connect to the server
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("[CONN]");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        // Construct the framed message
        snprintf(buffer, sizeof(buffer),"%s",payload);

        // Send the framed message
        send(client_socket, buffer, strlen(buffer), 0);

        printf("\n[Sending message to server...]\n[Message]: %s\n", buffer);

        // Wait for the server's response
        ssize_t bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);

        if (bytes_received > 0) {
            response[bytes_received] = '\0'; // Null-terminate the response
            printf("\n[Server Response]: %s\n", response);

            if (useIncrementals > 0 && (strlen(payload) + 1) < BUFFER_SIZE) {
                // Increment the payload size by appending 'X'
                size_t current_length = strlen(payload);
                payload[current_length] = 'X';  // Add an extra character
                payload[current_length + 1] = '\0'; // Null-terminate
                printf("\n[Incremented Payload]: %s\n", payload);
            }
             printf("\n[Server Response]: %s\n[currentMaxPayloadSize]: %lu\n", response, strlen(payload));
        } else if (bytes_received == 0) {
            printf("\n<<null byte received! connection reset>>\n");
        } else {
            printf("\n<<OH! something else happened, let's check the error stack>> [%zd] bytes received\n", bytes_received);
            perror("ERROR IN STACK");
        }

        // Close the socket
       close(client_socket);


        if (sleep_s_timeout > 0) {
            printf("\n[Sleeping for %d seconds...]\n", sleep_s_timeout);
            sleep(sleep_s_timeout);
        } else {
            sleep(0);
        }
    }

    return 0;
}
