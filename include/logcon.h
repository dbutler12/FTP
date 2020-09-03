#ifndef LOGCON_H
#define LOGCON_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct log { // Used for a linked list of logs of who connected
    char *host;
    int num;
    int process;
    int socket;
    struct log *next;
} Log;

Log *newLog(char *host, int process, int sock);
void freeLogs(Log *list);
Log *logCon(char *host, int process, int sock, Log *past);

#endif
