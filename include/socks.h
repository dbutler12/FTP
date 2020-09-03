#ifndef SOCKS_H
#define SOCKS_H

#include "const.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*#include <sys/socket.h>
#include <sys/un.h>#include <sys/types.h>*/

// Server/Client Cross Methods
int closeDataCon(int sock);

// Server Methods
int establishServDataCon(int sock, int *dataSocket);
void sendError(int sock, char *errString);
void sendAck(int sock,char *ack);
char getCommand(int sock, char *path);

// Client Methods
int establishClientDataCon(int sock, int *dataSock, const char *address);
char getAck(int sock);
void sendServCom(int sock, char com, char *path);

#endif
