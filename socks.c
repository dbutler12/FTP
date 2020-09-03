#include "socks.h"

// Closes socket
int closeDataCon(int sock){
    close(sock);
    return 0;
}

/*
 * Server Methods
 */

// Send error in the proper acknowledgement format
void sendError(int sock, char *errString){
    char sendError[2+strlen(errString)];
    
    fprintf(stderr,"Child %d: Failed with error: %s\n", getpid(), errString);
    sprintf(sendError,"E%s\n",errString);
    if(write(sock,sendError,strlen(sendError)) < 0 ){ 
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    fflush(NULL);
}

// Send acknowledgement in the proper form
void sendAck(int sock,char *ack){
    char sent[12];
    
    sprintf(sent,"A%s",ack);
    if(write(sock,sent,strlen(sent)) < 0 ){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    fflush(NULL);
}

// Get command and parse command from socket
char getCommand(int sock, char *path){
    int numberRead;
    char c, com;
    int i = 0;
    
    numberRead = read(sock,&com,1);
    if(numberRead > 0){
        numberRead = read(sock,&c,1);
    }else{
        if(numberRead < 0){ // Read error
            fprintf(stderr,"Error:%s\n",strerror(errno));
        }
        return 'Q';
    }
    
    while(c != '\n' && numberRead > 0 && i < PATHLEN){
        path[i] = c;
        i++;
        numberRead = read(sock,&c,1);
    }
    
    path[i] = '\0';
    
    return com;
}

// Create the data connection
// Sends port number to sock, then listens on that part to create dataSocket
int establishServDataCon(int sock, int *dataSocket){
    
    struct addrinfo hints, *servData;
    struct sockaddr_in servAddr;
    
    memset(&hints,0,sizeof(hints));
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(0);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // Create socket and connect
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        sendError(sock,strerror(errno));
        exit(1);
    }
    
    if( bind(socketfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0 ){
        sendError(sock,strerror(errno));
        exit(1);
    }
    
    struct sockaddr_in tempAddr;
    int addrLen;
    addrLen = sizeof(tempAddr);
    if (getsockname(socketfd, (struct sockaddr *) &tempAddr, &addrLen) == -1) {
        sendError(sock,strerror(errno));
        return -errno;
    }
    
    char dPort[10];
    sprintf(dPort,"%d\n",ntohs(tempAddr.sin_port));
    sendAck(sock,dPort);
    
    listen(socketfd, 1);
    
    int length = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    
    *dataSocket = accept(socketfd, (struct sockaddr *) &clientAddr, &length);
    
    if(*dataSocket < 0){
        sendError(sock,strerror(errno));
        exit(1);
    }
    
    return 1;
}


/* 
 * Client methods
 */

// Gets acknowledgements from server
// If an error, prints the error to stderr
// Returns the A or E
char getAck(int sock){
    char servRet[100] = {0};
    read(sock,servRet,100);

    if(servRet[0] == 'E'){ // Got an error
        char *errorStr = servRet;
        fprintf(stderr,"Error response from server: %s",++errorStr);
    }
    
    return servRet[0];
}

// Sends communications to server in the form %c%s\n
void sendServCom(int sock, char com, char *path){
    char temp[PATHLEN];
    sprintf(temp,"%c%s\n",com,path);
    if(write(sock,temp,strlen(temp)) < 0 ){
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    fflush(NULL);
}

// Establish data connection on the client side
// Listens for the socket, then tries to connect on that socket
int establishClientDataCon(int sock, int *dataSock, const char *address){
    char buf[100];
    int numRead;
    
    if((numRead = read(sock,buf,100)) <= 0){
        fprintf(stderr,"%s\n",strerror(errno));
        return -1;
    }
    
    if(buf[0] != 'A'){ // Error on server
        return -1;
    }
    
    char *por = buf;
    int dPort = atoi(++por);
    sprintf(buf,"%i",dPort);
    
    // Set up server address
    struct addrinfo hints, *servData;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    
    int err = getaddrinfo(address,buf,&hints,&servData);
    
    if(err != 0) {
        printf("Error: %s\n", gai_strerror(err));
        return -1;
    }
    
    // Create socket and connect
    *dataSock = socket(servData->ai_family, servData->ai_socktype, 0);
    
    if(connect(*dataSock, servData->ai_addr, servData->ai_addrlen) < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return -1;
    }
    
    return 1;
}
