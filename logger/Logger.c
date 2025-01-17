//
// Created by BaseDCaTx on 1/16/2025.
//

#include <stdio.h>
#include <stdlib.h>

void LogErrorWithReason(const char *reason, const char *details) {
    fputs(reason, stdout);
    fputs(" : ", stdout);
    fputs(details, stdout);
    fputs("\n", stdout);
}

void LogErrorWithReasonX(const char *reason, const char *details) {
    fputs(reason, stdout);
    fputs(" : ", stdout);
    fputs(details, stdout);
    fputs("\n", stdout);
    exit(EXIT_FAILURE);
}

void LogSystemError(const char *reason) {
    perror(reason);
    fputs("\n", stdout);
    exit(EXIT_FAILURE);
}