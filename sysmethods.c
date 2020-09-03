#include "sysmethods.h"

// Connects two passed in functions with a pipe
// Forks then closes the unused file descriptors
// Dupes stdout and saves it, for use if execvp fails
// Inputs:
//        args1 and args2 are arrays of strings passed to execvp
//        args1[0] and args2[0] should be the name of the file being used
void piped(char **args1, char **args2, int in, int out){
    int fd[2];
    int rdr, wtr, stdout;
    
    stdout = dup(1); // Store stdout in case it is closed, to print errors
    
    pipe(fd);
    
    rdr = fd[0]; wtr = fd[1];
    
    if(fork()){
        close(wtr);
        close(0); dup(rdr); close(rdr);
        if(out != -1) { close(1); dup(out); } 
        execvp(args2[0], args2);
        if(errno != 0){
            printf("%s\n", strerror(errno));
            exit(0);
        }
    }else{
        close(rdr);
        close(1); dup(wtr); close(wtr);
        if(in != -1) { close(0); dup(in); }
        execvp(args1[0], args1);
        if(errno != 0){
            printf("%s\n", strerror(errno));
            exit(0);
        }
    }
}

// More command for data sent through the dataSock
void more(int dataSock){
    int numberRead;
    char c;
    int fd[2]; 
    pipe(fd); // pipe used to know when child process is finished
    char *more[] = { "more", "-20", 0 };
    
    int fC = fork();
    
    if(fC){ // parent waits for child to finish
        close(fd[1]);
        while(read(fd[0],&c,1) && c != -1); // Wait on children
        close(fd[0]);
    }else{ // child to ls
        close(fd[0]);
        close(0);
        dup(dataSock);
        close(dataSock);
        execvp(more[0], more);
        exit(0);
    }
}

// Function for calling ls
// If client calls (dataSock == -1), uses the piped function to make a pipe between ls and more
// If server calls (dataSock != -1), dups the dataSocket into the stdout slot then execvps ls
int ls(int dataSock){
    char c;
    int fd[2]; 
    pipe(fd); // pipe used to know when child process is finished
    char *lser[] = { "ls", "-l", 0 };
    char *more[] = { "more", "-20", 0 };
    
    int fC = fork();
    
    if(fC){ // parent waits for child to finish
        close(fd[1]);
        while(read(fd[0],&c,1)); // Wait on children
        close(fd[0]);
    }else{ // child to ls
        close(fd[0]);
        if(dataSock == -1){
            piped(lser, more, -1, -1);
        }else{
            close(1);
            dup(dataSock);
            close(dataSock);
            execvp(lser[0], lser);  
        }
        exit(0);
    }
}

// Function to check if a file is valid, not a directory, and we have access
// Returns 1 on success, 0 on failure.
int checkFile(int sock, char *path){
    struct stat area, *s = &area;

    if(lstat(path,s) == 0){
        if(S_ISREG(s->st_mode) && (s->st_mode & S_IRUSR)){  // Found a file
            return 1;
        }else if(S_ISDIR(s->st_mode)){ // Found a directory
            if(sock == -1) fprintf(stderr,"Cannot send file, is directory.\n");
            if(sock != -1) sendError(sock,"Cannot send file, is directory.");
            return 0;
        }else{
            if(sock == -1) fprintf(stderr,"Error:Cannot access file.\n");
            if(sock != -1) sendError(sock, "Cannot access file.");
            return 0;
        }
    }else{
        if(sock == -1) fprintf(stderr,"%s\n",strerror(errno));
        if(sock != -1) sendError(sock,strerror(errno));
        return 0;
    }
    return 1;
}

// Function to change directory
// If client calls, (sock == -1), errors are sent to stderr
// If server calls, (sock != -1), errors are sent to sock
int changeDir(int sock, char *path){
    struct stat area, *s = &area;

    if(lstat(path,s) == 0){
        if(S_ISREG(s->st_mode)){  // Found a file
            if(sock == -1) fprintf(stderr,"Error:Not a directory.\n");
            if(sock != -1) sendError(sock,"Not a directory.");
            return 0;
        }else if(S_ISDIR(s->st_mode) && (s->st_mode & S_IRUSR)){ // Found a directory
            if(chdir(path) != 0){
                if(sock == -1) fprintf(stderr,"%s",strerror(errno));
                if(sock != -1) sendError(sock,strerror(errno));
                errno = 0;
                return 0;
            }
        }else{
            if(sock == -1) fprintf(stderr,"Error:Cannot access directory.\n");
            if(sock != -1) sendError(sock,"Cannot access directory.");
            return 0;
        }
    }else{
        if(sock == -1) fprintf(stderr,"%s\n",strerror(errno));
        if(sock != -1) sendError(sock,strerror(errno));
        return 0;
    }
    return 1;
}

// Function to send files through dataSock
// If client calls, (sock == -1), errors are sent to stderr
// If server calls, (sock != -1), errors are sent to sock
void sendFile(int sock, int dataSock, char *path){
    char buffer[1000];
    int numberRead;
    
    int fd = open(path,O_RDONLY); // Only need to read
    
    if(fd < 0){ // Failure to open
        if(sock == -1) fprintf(stderr,"Error:%s %s\n",strerror(errno),path);
        if(sock != -1) sendError(sock, strerror(errno));
        return;
    }
    
    if(sock != -1) sendAck(sock,"\n");
    
    int count = 0;
    
    while((numberRead = read(fd,buffer,1000)) > 0 && buffer[0] != -1){ // If we read something, and it is not EOF, continue
        if(write(dataSock,buffer,numberRead) < 0){ // Write file contents to dataSock
            fflush(NULL);
            break;
        }
        fflush(NULL);
    }
}

// Function to recieve files through dataSock
// If client calls, (sock == -1), errors are sent to stderr
// If server calls, (sock != -1), errors are sent to sock
void recieveFile(int sock, int dataSock, char *path, char *storePath){
    char *filename = findFilename(path);
    int fd = open(filename,O_CREAT | O_EXCL | O_WRONLY,00700); // Creating a file with permissions rwx for user, to write to
    int numberRead;
    char buf[1000];
    
    if(fd < 0){ // Failure to open
        if(sock == -1) fprintf(stderr,"Error:%s\n",strerror(errno));
        if(sock != -1) sendError(sock, strerror(errno));
        free(filename);
        return;
    }
    
    if(sock != -1) sendAck(sock,"\n");
    
    while((numberRead = read(dataSock,buf,1000)) > 0){ 
        write(fd,buf,numberRead); // Write to file if there is stuff to read
        fflush(NULL);
    }
    
    close(fd);
    free(filename);
}

// Finds the filename at the end of a path
char *findFilename(char *path){
    char name[100];
    int len = strlen(path);
    int i = 0, j = 0; // i is the index of path, j is the index of stored filename
    while(path[i] != '\0' && i < len){
        char c = path[i];
        if(c == '/'){ // If slash is found, reset index of stored filename
            j = 0;
            i++;
            memset(name,0,100);
            continue;
        }
        name[j++] = path[i++];
    }
    
    name[j] = '\0';
    
    char *final = malloc(j*sizeof(char));
    strcpy(final,name);
    
    return final;
}
