#include "logcon.h"

// Sets aside memory for a new log
Log *newLog(char *host, int process, int sock){ 
    Log *newlog = malloc(sizeof(struct log));
    newlog->host = malloc(strlen(host));
    strcpy(newlog->host,host);
    newlog->process = process;
    newlog->socket = sock;
    newlog->num = 1;
    newlog->next = NULL;
    return newlog;
}

// Frees the list upon completion of the server
void freeLogs(Log *list){ 
    while(list != NULL){
        Log *temp = list;
        list = list->next;
        free(temp->host);
        free(temp);
    }
}

// Logs a connection by traversing the list and inserting data
Log *logCon(char *host, int process, int sock, Log *past){ 
    Log *temp = past;
    
    while(strcmp(host, temp->host) != 0 && temp->next != NULL && temp->process != process && temp->socket != sock){
        temp = temp->next;
    }
    
    if(strcmp(host,temp->host) == 0  && temp->process == process && temp->socket == sock){
        temp->num += 1;
    }else{
        temp->next = newLog(host,process,sock);
        return temp->next;
    }
    
    return temp;
}
