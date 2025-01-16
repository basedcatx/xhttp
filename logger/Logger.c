//
// Created by BaseDCaTx on 1/16/2025.
//

#include <stdio.h>
#include <stdlib.h>

void LogErrorWithReason(const char *reason, const char *details) {
    puts(reason);
    puts(" : ");
    puts(details);
    puts("\n");
}

void LogErrorWithReasonX(const char *reason, const char *details) {
    puts(reason);
    puts(" : ");
    puts(details);
    puts("\n");
    exit(EXIT_FAILURE);
}

void LogSystemError(const char *reason) {
    perror(reason);
    puts("\n");
    exit(EXIT_FAILURE);
}