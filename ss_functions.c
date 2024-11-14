#include "./storageserver.h"
// #include "./requests.h"
#include "./commonheaders.h"
#include "namingserver.h"
#include "./ss_functions.h"
#include <errno.h>
// #include "storageserver.c"
// int itemcount = 0;
 int ns_sockfd;
void* send_err_to_ns(int Socket,char *msg)
{
    request pack;
    pack.requestType = ERROR;
    strcpy(pack.data, msg);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(Socket, buffer, sizeof(request), 0) < 0){
        perror("Send failed");
        close(Socket);
        return NULL;
    }
}
void* send_ack_to_ns(int Socket,char *msg)
{
    request pack;
    pack.requestType = ACK;
    strcpy(pack.data, msg);
    printf("ayush bhai ack karo yaar\n");
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));

    if(send(Socket, buffer, sizeof(request), 0) < 0){
        perror("Send failed");
        close(Socket);
        return NULL;
    }
}

#define PORT 8083
#define CLIENT_PORT 8084
// function to handle ns request
void * handle_ns_req(void* arg){
    int clientSocket = *(int*)arg;
    char buffer[sizeof( request)];
    memset(buffer, 0, sizeof(request));
    if(recv(clientSocket, buffer, sizeof( request), 0) < 0){
        perror("Receive failed");
        
        send_err_to_ns(clientSocket,"Receive failed");
        close(clientSocket);
        return NULL;
    }
    request req;
    memset(&req, 0, sizeof(request));
    memcpy(&req, buffer, sizeof( request));
    printf("Received request from naming server\n");
    // check the request type
    if(req.requestType==CREATEFOLDER)
    {
        // create the folder
        printf("Creating folder\n");
        printf("Folder name: %s\n", req.data);
        if(mkdir(req.data, 0777) < 0){
            perror("Folder creation failed");
            send_err_to_ns(clientSocket,"Folder creation failed");
            close(clientSocket);
            return NULL;
        }
        printf("Folder created successfully\n");
        send_ack_to_ns(clientSocket,"Folder created successfully");
    }
    else if(req.requestType==DELETEFOLDER)
    {
        // delete the folder
        printf("Deleting folder\n");
        printf("Folder name: %s\n", req.data);
        if(rmdir(req.data) < 0){
            perror("Folder deletion failed");
            close(ns_sockfd);
            return NULL;
        }
        printf("Folder deleted successfully\n");
        send_ack_to_ns(clientSocket,"Folder deleted successfully");
    }
    else if(req.requestType==CREATEFILE)
    {
        // create the file
        printf("Creating file\n");
        printf("File name: %s\n", req.data);
        FILE *file = fopen(req.data, "w");
        if(file==NULL){
            perror("File creation failed");
            close(clientSocket);
            send_err_to_ns(clientSocket,"File creation failed");
            return NULL;
        }
        fclose(file);
        printf("File created successfully\n");
        send_ack_to_ns(clientSocket,"File created successfully");
    }
    else if(req.requestType==DELETEFILE)
    {
        // delete the file
        printf("Deleting file\n");
        printf("File name: %s\n", req.data);
        if(unlink(req.data) < 0){
            perror("File deletion failed");
            send_err_to_ns(clientSocket,"File deletion failed");
            close(clientSocket);
            return NULL;
        }
        printf("File deleted successfully\n");
        send_ack_to_ns(clientSocket,"File deleted successfully");
    }
    // else if(req.requestType==LIST)
    // {
    //     // list the folder
    //     printf("Listing folder\n");
    //     printf("Folder name: %s\n", req.data);
    //     DIR *dir = opendir(req.data);
    //     if(dir==NULL){
    //         perror("Folder not found");
    //         send_err_to_ns(clientSocket,"Folder not found");
    //         close(clientSocket);
    //         return NULL;
    //     }
    //     struct dirent *entry;
    //     char data[1024];
    //     memset(data, 0, 1024);
    //     while((entry=readdir(dir)) != NULL){
    //         strcat(data, entry->d_name);
    //         strcat(data, "\n");
    //     }
    //     closedir(dir);
    //     printf("Sending folder list to naming server\n");
    //     request pack;
    //     pack.requestType = LIST;
    //     strcpy(pack.data, data);
    //     char buffer[sizeof(request)];
    //     memset(buffer, 0, sizeof(request));
    //     memcpy(buffer, &pack, sizeof(request));
    //     send
    // }
        
   itemcount--;



}

// function to start   and bind the socket to the port for listening to the naming server  
void* NS_listener(void* arg){
      struct sockaddr_in serv_addr;
     
        // Create socket
    ns_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ns_sockfd < 0) {
        perror("ERROR opening socket");
        return NULL;
    }
    // Prepare server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    setsockopt(ns_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    // Bind the socket
    if (bind(ns_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(ns_sockfd);
        return NULL;
    }
    printf("Bind to port %d successful...\n", SERVER_PORT);
    // listen to the naming server
    if(listen(ns_sockfd, MAX_CLIENTS) < 0){
        perror("Listen failed");
        close(ns_sockfd);
        return NULL;
    }
    for(;;){
        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);

        int clientSocket = accept(ns_sockfd, (struct sockaddr *)&clientaddr, &len);
        if(clientSocket < 0){
            perror("Client accept failed");
            close(ns_sockfd);
            return NULL;
        }
        printf("Client connected\n");
        // handle the client request
        if(itemcount<MAX_CLIENTS){
            pthread_t tid;
            itemcount++;
            pthread_create(&tid, NULL, handle_ns_req, &clientSocket);
        }
        else{
            printf("Server is full\n");
            close(clientSocket);
        }
        
    }

}
// function to handle client request
void* handle_client_req(void* arg)
{
    int clientSocket = *(int*)arg;
    char buffer[sizeof( request)];
    memset(buffer, 0, sizeof(request));
    if(recv(clientSocket, buffer, sizeof(request), 0) < 0){
        perror("Receive failed");
        
        close(clientSocket);
        return NULL;
    }
    request req;
    memset(&req, 0, sizeof( request));
    memcpy(&req, buffer, sizeof( request));
    printf("Received request from client\n");
    printf("Request type: %d\n", req.requestType);
    // check the request type
    if(req.requestType==READ){
        // read the file

        char *filename = req.data;
        FILE *file = fopen(filename, "r");
        if(file==NULL){
            perror("File not found");
            close(clientSocket);
            send_err_to_ns(clientSocket,"File not found");
            return NULL;
        }
        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *filedata = (char*)malloc(size);
        fread(filedata, 1, size, file);
        fclose(file);
        printf("Sending file data to client\n");
        request pack;
        pack.requestType = READ;
        strcpy(pack.data, filedata);
        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(request));
        memcpy(buffer, &pack, sizeof(request));
        // send the file data to the client
       if(send(clientSocket, buffer, sizeof(request), 0) < 0){
            perror("Send failed");
            close(clientSocket);
            return NULL;
        }
        free(filedata);


    }
    else if(req.requestType==WRITEASYNC){
        // write the file
        printf("Writing file\n");
        printf("Data: %s\n", req.data);
        char* tok=strtok(req.data, "\n");
        char filename[strlen(tok)+1];
        strcpy(filename, tok);
        printf("Filename: %s\n", filename);
        tok=strtok(NULL, "\n");
        char data[strlen(tok)+1];
        strcpy(data, tok);
        FILE *file = fopen(filename, "a");
        if(file==NULL){
            perror("File not found");
            close(clientSocket);
            return NULL;}
             
        
        fprintf(file, "%s", data);

        fclose(file);
        printf("File written successfully\n");
       
        }   
    else if(req.requestType==STREAM){
          FILE*  file = fopen(req.data, "rb");
          printf("Audio name: %s\n", req.data);
        if(file==NULL){
            perror("File not found");
            close(clientSocket);
            return NULL;}
        char data[MAX_STRUCT_LENGTH-2];
        memset(data, 0, MAX_STRUCT_LENGTH);
        size_t nread;
        while((nread = fread(data, 1, MAX_STRUCT_LENGTH, file)) > 0){
            request pack;
            pack.requestType = STREAM;
            strcpy(pack.data, data);
            char buffer[sizeof(request)];
            memset(buffer, 0, sizeof(request));
            memcpy(buffer, &pack, sizeof(request));
            if(send(clientSocket, buffer, sizeof(request), 0) < 0){
                perror("Send failed");
                close(clientSocket);
                return NULL;
            }
            memset(data, 0, MAX_STRUCT_LENGTH);
        }
        printf("Audio File sent successfully\n");
        
    } 

        itemcount--;

    
}
// function to listen the client request
void * Client_listner(void * arg)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return NULL;
    }
    // Prepare server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(CLIENT_PORT);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return NULL;
    }
    printf("Bind to port %d successful...\n", CLIENT_PORT);
    // listen to the naming server
    if(listen(sockfd, MAX_CLIENTS) < 0){
        perror("Listen failed");
        close(sockfd);
        return NULL;
    }
    for(;;){
        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);

        int clientSocket = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
        if(clientSocket < 0){
            perror("Client accept failed");
            close(sockfd);
            return NULL;
        }
        printf("Client connected\n");
        // handle the client request
        if(itemcount<MAX_CLIENTS){
            pthread_t tid;
            itemcount++;
            pthread_create(&tid, NULL, handle_client_req, &clientSocket);
        }
        else{
            printf("Server is full\n");
            close(clientSocket);
        }
        
    }
}

