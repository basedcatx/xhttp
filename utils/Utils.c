//
// Created by BaseDCaTx on 1/20/2025.
//

#include <time.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include "../includes/utils.h"

void generate_http_header(char *header, size_t header_size) {
    // Seed the random number generator
    srand(time(NULL));

    // Generate a random Content-Length between 100 and 999
    int content_length = rand() % (MAX_CONTENT_LENGTH - MIN_CONTENT_LENGTH + 1) + MIN_CONTENT_LENGTH;

    // Format the HTTP header with the randomized Content-Length
    snprintf(header, header_size, HTTP_HEADER_TEMPLATE, content_length);
}
