/*
 * MFTP Client Program
 * Author: Dwayne Butler (Saanail.Vernath@gmail.com)
 */

#include "client.h"

// Function to split input into command and path
void splitInput(char *input, char *command, char *path){
    int split = 0;
    int i = 0;
    path[0] = '\0';
    while(input[i] != '\0' && i < INPUT){
        if(split == 0){ // Split not found yet
            if(i ==  MAXCOMLEN){ // No split found, too much input
                command[i - 1] = '\0';
                break;
            }
            if(input[i] == ' '){ // Found the space to split on
                split = i + 1;
                command[i] = '\0';
            }else if(input[i] == '\n'){
                command[i] = '\0';
                break;
            }else{ // Split not found, add to command string
                command[i] = input[i];   
            }
        }else{ // Split found, now create the path string
            if(input[i] == '\n' || input[i] == ' ') break;
            path[i - split] = input[i];
        }
        
        i++;
    }

    path[i - split] = '\0';
}

// Hash function from assignment 1
// Uses a polynomial function similar to Horner's algorithm to create an int from a string
unsigned int stringHash(const unsigned char *string){  
    int len = strlen(string);
    if(len > MAXCOMLEN) return -1; // -1 tells us that this isn't a valid command
    
    unsigned int hashSize = 2147483647;
    unsigned int length = MAXCOMLEN;
    unsigned int code = 0;
    unsigned int prime = 37;
    
    int i = 0;
    while(i < length){
        if(i >= len){
            code = (code * prime + 'a');
            i++;
        }else{
            code = (code * prime + string[i++]);
        }
    }
    
    return code % hashSize; // mod the solution down to the hash size we are using
}


// Function to take commands from the given socket
// Pass in used address for the socket as well for use with the datasocket
int command(int sock,const char *address){
    
    char input[INPUT];
    char command[MAXCOMLEN];
    char path[INPUT - MAXCOMLEN];
    char servCom[PATHLEN+1];
    
    int com = 0;
    
    int dataSock;
    int dataUp = 0;
    char servRet;
    
    char storePath[PATHLEN] = "./";
    int len;
    
    while(com != EXIT){
        memset(input,0,INPUT); // Reset input buffer to clear out old data
        printf("MFTP> ");
        fflush(stdout); // Flush remaining bits from stdout, to be sure there are no issues
        int numRead = read(1,input,INPUT); // Read input based on INPUT size
        if(numRead > 0 && strlen(input) >= 2 && isalpha(input[0]) && isalpha(input[1])){ // Check if input is valid
            splitInput(input,command,path); // Split input into command and path
            com = stringHash(command);     // hash the command for ease of use with below switch statement
        }
        switch(com){
            case CD:
                if(changeDir(-1,path) < 0) fprintf(stderr,"Invalid directory.\n");
                break;
            case LS:
                ls(-1);
                break;
            case RCD:
                sendServCom(sock,'C',path);
                getAck(sock);
                break;
            case RLS:
                sendServCom(sock,'D',"");
                dataUp = establishClientDataCon(sock,&dataSock,address);
                if(dataUp){
                    sendServCom(sock,'L',"");
                    more(dataSock);
                    dataUp = closeDataCon(dataSock);
                }
                getAck(sock);
                break;
            case GET:
                sendServCom(sock,'D',"");
                dataUp = establishClientDataCon(sock,&dataSock,address);
                if(dataUp){
                    sendServCom(sock,'G',path);
                    if(getAck(sock) == 'A') recieveFile(-1,dataSock,path,storePath);
                    dataUp = closeDataCon(dataSock);
                }
                break;
            case SHOW:
                sendServCom(sock,'D',"");
                dataUp = establishClientDataCon(sock,&dataSock,address);
                if(dataUp){
                    sendServCom(sock,'G',path);
                    if(getAck(sock) == 'A') more(dataSock);
                    dataUp = closeDataCon(dataSock);
                }
                break;
            case PUT:
                if(checkFile(-1, path) == 0) break;
                sendServCom(sock,'D',"");
                dataUp = establishClientDataCon(sock,&dataSock,address);
                if(dataUp){
                    sendServCom(sock,'P',path);
                    if(getAck(sock) == 'A') sendFile(-1,dataSock,path);
                    dataUp = closeDataCon(dataSock);
                }
                break;
            case EXIT:
                sendServCom(sock,'Q',"");
                getAck(sock);
                exit(0);
            default:
                fprintf(stderr,"Invalid Command.\n");
                break;
        }
    }
    
}

// Client program to connect to the server
// Takes a part and an address, then runs through the necessary socket setup to connect
void client(const char *port, const char *address){
    int done = 0;
    int socketfd;
    
    // Set up server address
    struct addrinfo hints, *servData;
    memset(&hints,0,sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    
    int err = getaddrinfo(address,port,&hints,&servData);
    
    if(err != 0) {
        printf("Error: %s\n", gai_strerror(err));
        exit(1);
    }
    
    // Create socket and connect
    socketfd = socket(servData->ai_family, servData->ai_socktype, 0);
    
    if(connect(socketfd, servData->ai_addr, servData->ai_addrlen) < 0){
        fprintf(stderr, "Error: %s\n", strerror(errno));
        exit(1);
    }
    
    // Start taking commands from the server using the passed socket
    command(socketfd,address);
}

int main(int argc, char *argv[]){
    
    if(argc == 2){
        char port[10];
        sprintf(port,"%d",PORT);
        client(port,argv[2]);
    }else if(argc == 3){
        client(argv[1],argv[2]);
    }else{
        printf("Wrong arguments.\n");
    }
    
    return 0;
}
