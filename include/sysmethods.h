#ifndef SYSMETHODS_H
#define SYSMETHODS_H

#include "socks.h"
#include "const.h"

#include <sys/stat.h>
#include <fcntl.h>

void piped(char **args1, char **args2, int in, int out);
int ls(int dataSock);
int changeDir(int sock, char *path);
void sendFile(int sock, int dataSock, char *path);
void recieveFile(int sock, int dataSock, char *path, char *storePath);
char *findFilename(char *path);
int checkFile(int sock, char *path);
void more(int dataSock);

#endif
