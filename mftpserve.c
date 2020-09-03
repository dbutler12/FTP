/*
 * MFTP Server Program
 * Author: Dwayne Butler (Saanail.Vernath@gmail.com)
 */

#include "server.h"

// Server control connection, to recieve commands from clients
// Accepts commands from sock
void controlCon(int sock){
    
    char con = '0';
    int dataUp = 0;
    char *dError = "ENo data connection established.\n";
    int dataSock;
    
    int count = 0;
    
    while(con != 'Q'){
        
        int suc = -1; // Success checker variable
        char path[PATHLEN];
        
        con = getCommand(sock,path);
        
        //printf("Command:%c\nPath:%s\n",con,path);
        //fflush(stdout);
        
        if(dataUp == 0 && (con == 'L' || con == 'G' || con == 'P')){ // Needs a data connection, but there isn't one
            sendError(sock,"No data connection established.");
            fflush(NULL);
        }else if(dataUp != 0 && (con == 'L' || con == 'G' || con == 'P')){ // Needs a data connection, and there is one
            switch(con){
                case 'L':
                    ls(dataSock);
                    dataUp = closeDataCon(dataSock);
                    sendAck(sock,"\n");
                    break;
                case 'G':
                    if(checkFile(sock,path) != 0) sendFile(sock, dataSock, path);
                    dataUp = closeDataCon(dataSock);
                    break;
                case 'P':
                    recieveFile(sock,dataSock,path,"");
                    dataUp = closeDataCon(dataSock);
                    break;
            }
        }else{ // Does not need a data connection
            switch(con){
                case 'Q': // Quit
                    sendAck(sock,"\n");
                    if(dataUp) closeDataCon(dataSock);
                    return;
                case 'D': // Data connection
                    dataUp = establishServDataCon(sock,&dataSock);
                    break;
                case 'C': // change directory
                    suc = changeDir(sock, path);
                    if(suc > 0) sendAck(sock,"\n");
                    break;
            }
        }
    }
}

// Creates a server that runs until you exit it with ctrl + c
// Maintains a socket for clients to connect to, then forks a process for them
// Logs who has connected
void server(int port){
    Log *list = newLog("head",0,0); // Create the list
    list->num = 0; // No one has connected yet, so set to 0
    char buf[100] = {0};
    char portC[10] = {0};
    int numberRead;
    
    int descriptor;
    int socketfd;
    struct addrinfo hints, *servData;
    struct sockaddr_in servAddr;
    
    sprintf(portC,"%i",port);
    memset(&hints,0,sizeof(hints));
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    
    // Set up address info
    int err = getaddrinfo("localhost",portC,&hints,&servData);
    
    if(err != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(err));
        freeLogs(list);
        exit(1);
    }
    
    // Create socket and connect
    socketfd = socket(servData->ai_family, servData->ai_socktype, 0);
    
    if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeLogs(list);
        exit(1);
    }
    
    if(bind(socketfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0 ){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        freeLogs(list);
        exit(1);
    }

    if(listen(socketfd, 4) < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
    }
    
    int connectfd;
    int length = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    
    // Wait for connections, when a connection occurrs, log it and setup a process for it
    while(1){
        connectfd = accept(socketfd, (struct sockaddr *) &clientAddr, &length);
        char buf[30];
        
        inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, buf, sizeof(buf));
        
        if(connectfd < 0){
            fprintf(stderr, "Error: %s\n", strerror(errno));
            freeLogs(list);
            exit(1);
        }//else{ Saved for my sake, to remind me what connectfd is doing
            // printf("Client Descriptor: %i\n", connectfd);
        //}
        
        char hostName[NI_MAXHOST];
        int hostEntry;
        
        // Find host name of connection
        hostEntry = getnameinfo((struct sockaddr *) &clientAddr, sizeof(clientAddr), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV);
        
        if(hostEntry != 0){
            printf("Error: %s\n", gai_strerror(errno));
            freeLogs(list);
            exit(1);
        }
        
        int split = fork();
        
        if(split < 0){
            fprintf(stderr,"Process creation failed. Closing client connection.\n");
            close(connectfd);
        }else if(split == 0){ // Child process
            controlCon(connectfd);
            printf("Child %d: Quitting\n", getpid());
            exit(0); // Child exits
        }
        
        // Parent process continues from fork()
        
        // Log the connection
        Log *host = logCon(hostName,split,connectfd,list);
        
        printf("Child %d: client IP address -> %s\n", split, buf);
        printf("Child %d: connection accepted from host %s\n", split, buf);
        fflush(stdout);
        
    }
    
    freeLogs(list);
}

int main(int argc, char const *argv[]){
    if(argc == 1){
        server(PORT);
    }else if(argc == 2){
        server(atoi(argv[1]));
    }else{
        printf("Wrong arguments.\n");
    }

    return 0;
}
