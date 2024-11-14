
#include "./namingserver.h"

// Send data to a port
int connectTo(int port, char* ip){
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }
    
    // Get server information
    server = gethostbyname(ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1;
    }

    // Set up server address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(8083);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        int errsv = errno;
        printf("errno: %d\n", errsv);
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void connectToSS(StorageServer ss){
    // connect to the storage server
    printf("Connecting to storage server %s:%d\n", ss.ssIP, ss.ssPort);
    int ssSocket;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss.ssPort);
    serv_addr.sin_addr.s_addr = inet_addr(ss.ssIP);

    ssSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ssSocket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    if (connect(ssSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Failed to connect to storage server");
        close(ssSocket);
        // exit(EXIT_FAILURE);
    }

    storageServersList[currentServerCount] = malloc(sizeof(StorageServer));
    // storageServersList[currentServerCount]->ssIP = malloc(strlen(ss.ssIP) + 1);
    strcpy(storageServersList[currentServerCount]->ssIP, ss.ssIP);
    
    storageServersList[currentServerCount]->ssPort = ss.ssPort;
    storageServersList[currentServerCount]->ssSocket = ssSocket;
    storageServersList[currentServerCount]->clientPort = ss.clientPort;
    printf("Connected to storage server %s:%d\n", ss.ssIP, ss.ssPort);
    storageServersList[currentServerCount]->root = create_trie_node();
    insert_path(storageServersList[currentServerCount]->root, "./ar.txt", currentServerCount);
    insert_path(storageServersList[currentServerCount]->root, ".", currentServerCount);
    insert_path(storageServersList[currentServerCount]->root, "./audio.wav", currentServerCount);
    storageServersList[currentServerCount]->numberOfPaths = 2;
    pthread_mutex_init(&storageServersList[currentServerCount]->mutex, NULL);
    storageServersList[currentServerCount]->status = 1;
    currentServerCount++;

    // send(ssSocket, "ACK", 3, 0);

    // char buffer[1024];

    // // Read accessible paths from the client
    // // int bytes_received = recv(ssSocket, buffer, sizeof(buffer) - 1, 0);
    // // if (bytes_received <= 0) {
    // //     perror("Failed to receive accessible paths");
    // //     close(ssSocket);
    // //     close(sockfd);
    // //     exit(EXIT_FAILURE);
    // // }
    // buffer[bytes_received] = '\0'; // Null-terminate the received string

    // // Parse and store accessible paths
    // char *token = strtok(buffer, ",");
    // while (token != NULL) {
    //     // addAccessiblePaths(token, currentServerCount - 1);
    //     token = strtok(NULL, ",");
    // }

    close(ssSocket);   
    return;
}

void handleWrite(processRequestStruct* req){
    client_request cr;
    memset(&cr, 0, sizeof(client_request));
    //req->data has path
    strcpy(cr.path, req->data);

    //find storage server with the path
    int checkPathIfPresent = 0;
    StorageServer* ss;
    for(int i = 0; i < currentServerCount; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);  
        if(search_path(storageServersList[i]->root, cr.path) >= 0){
            checkPathIfPresent = 1;
            ss = storageServersList[i];
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            break;
        }
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    if(checkPathIfPresent == 0){
        printf("Path not found\n");
        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Path not found");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        return;
    }   

    //send ip and port of ss to client for them to directly connect
    request req2;
    memset(&req2, 0, sizeof(request));
    req2.requestType = ACK;
    char data[MAX_STRUCT_LENGTH];
    snprintf(data, MAX_STRUCT_LENGTH, "%s %d", ss->ssIP, ss->clientPort);
    strcpy(req2.data, data);
    printf("%s %d", req2.data, req2.requestType);

    char buffer3[sizeof(request)];
    memset(buffer3, 0, sizeof(buffer3));
    memcpy(buffer3, &req2, sizeof(request));

    int ch2 = send(clientSockets[req->clientID], buffer3, sizeof(request), 0);
    if(ch2 < 0){
        printf("Failed to send response to client");
    }

    return;
}

void handleDelete(processRequestStruct* req){
    client_request cr;
    memset(&cr, 0, sizeof(client_request));
    //req->data has path
    strcpy(cr.path, req->data);
    printf("Path: %s\n", cr.path);

    //find storage server with the path
    int checkPathIfPresent = 0;
    StorageServer* ss;
    for(int i = 0; i < currentServerCount; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);  
        if(search_path(storageServersList[i]->root, cr.path) >= 0){
            checkPathIfPresent = 1;
            ss = storageServersList[i];
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            break;
        }
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    if(checkPathIfPresent == 0){
        printf("Path not found\n");
        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Path not found");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        return;
    }   

    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Failed to connect to storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }

    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    memcpy(data, &cr, sizeof(client_request));

    request reqq;
    memset(&reqq, 0, sizeof(request));
    reqq.requestType = req->requestType;
    memcpy(reqq.data, data, sizeof(client_request));

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &reqq, sizeof(request));

    int ch = send(sockfd, buffer, sizeof(request), 0);

    if(ch < 0){
        printf("Failed to send request to storage server");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;
        strcpy(req2.data, "Failed to send request to storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);

    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server\n");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Failed to receive response from storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }

    request res;
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    if(res.requestType == ACK){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        delete_path(ss->root, cr.path);
        ss->numberOfPaths--;
        pthread_mutex_unlock(&ss->mutex);

        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ACK;
        strcpy(req2.data, "File/Folder deleted successfully");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
    }
    else{
        printf("Failed to delete file/folder\n");
        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Failed to delete file/folder");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
    }

    close(sockfd);
    return;
}

// handles the Create File/Folder request, takes the request struct as input, searches for the path in the storage servers and sends the request to the appropriate storage server
void handleCreate(processRequestStruct* req){
    client_request cr; 
    memset(&cr, 0, sizeof(client_request));
    // req->data has name and path as name|path
    char* token = strtok(req->data, " ");
    strcpy(cr.path, token);
    token = strtok(NULL, " ");
    strcpy(cr.name, token);

    //find storage server with least number of paths
    int checkPathIfPresent = 0;
    StorageServer* ss;
    int min = 100000000;
    for(int i = 0; i < currentServerCount; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);  
        if(storageServersList[i]->numberOfPaths < min){
            min = storageServersList[i]->numberOfPaths;
            ss = storageServersList[i];
            checkPathIfPresent = 1;
        }
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }

    if(checkPathIfPresent == 0){
        printf("Path not found\n");
        return;
    }

        // check if the file/folder already exists
        if(search_path(ss->root, cr.name) >= 0){
            printf("File/Folder already exists\n");
            //sending this to client
            request req2;
            memset(&req2, 0, sizeof(request));
            req2.requestType = ERROR;

            strcpy(req2.data, "File/Folder already exists");

            char buffer[sizeof(request)];
            memset(buffer, 0, sizeof(buffer));
            memcpy(buffer, &req2, sizeof(request));

            int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

            if(ch < 0){
                printf("Failed to send response to client");
            }
            return;
        }

    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Failed to connect to storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }

    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    memcpy(data, &cr, sizeof(client_request));

    request reqq;
    memset(&reqq, 0, sizeof(request));
    reqq.requestType = req->requestType;
    memcpy(reqq.data, data, sizeof(client_request));

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &reqq, sizeof(request));

    int ch = send(sockfd, buffer, sizeof(request), 0);
    
    if(ch < 0){
        printf("Failed to send request to storage server");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;
        strcpy(req2.data, "Failed to send request to storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);
    

    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server");
        ss->status = 0;

        //sending this to client
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;

        strcpy(req2.data, "Failed to receive response from storage server");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);

        if(ch < 0){
            printf("Failed to send response to client");
        }
        close(sockfd);
        return;
    }
    
    request res; 
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    if(res.requestType == ACK && (req->requestType == CREATEFILE || req->requestType == CREATEFOLDER)){
        printf("File/Folder created successfully\n");
        // add path to the trie
        char path[MAX_PATH_LENGTH];
        memset(path, 0, sizeof(path));
        sprintf(path, "%s/%s", cr.path, cr.name);
        pthread_mutex_lock(&ss->mutex);
        insert_path(ss->root, path, currentServerCount);
        ss->numberOfPaths++;
        pthread_mutex_unlock(&ss->mutex);

        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ACK;
        strcpy(req2.data, "File/Folder created successfully");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);
        if(ch < 0){
            printf("Failed to send response to client");
            // close(clientSockets[req->clientID]);
            return;
        }

    }
    else if(res.requestType == ACK && (req->requestType == DELETEFILE || req->requestType == DELETEFOLDER)){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        delete_path(ss->root, cr.path);
        pthread_mutex_unlock(&ss->mutex);

        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ACK;
        strcpy(req2.data, "File/Folder deleted successfully");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);
        if(ch < 0){
            printf("Failed to send response to client");
            // close(clientSockets[req->clientID]);
            return;
        }
    }
    else if(res.requestType == ERROR){
        printf("Error processing request\n");
        request req2;
        memset(&req2, 0, sizeof(request));
        req2.requestType = ERROR;
        strcpy(req2.data, "Error processing request");

        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, &req2, sizeof(request));

        int ch = send(clientSockets[req->clientID], buffer, sizeof(request), 0);
        if(ch < 0){
            printf("Failed to send response to client");
            // close(clientSockets[req->clientID]);
            return;
        }
    }

    close(sockfd);
    return;
}

// Thread function to process requests
void* processRequests(void* args){
    processRequestStruct* req = (processRequestStruct*)args;
    printf("Processing request\n");
    if(req->requestType == INITSS){
        StorageServer ss;
        memset(&ss, 0, sizeof(StorageServer));
        memcpy(&ss, req->data, sizeof(StorageServer));
        printf("Connecting to storage server %s:%d\n", ss.ssIP, ss.ssPort);
        connectToSS(ss);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    else if(req->requestType == CREATEFOLDER || req->requestType == CREATEFILE){
        // create file
        handleCreate(req);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    else if(req->requestType == DELETEFOLDER || req->requestType == DELETEFILE){
        // delete file
        handleDelete(req);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    else if(req->requestType == WRITESYNC || req->requestType == WRITEASYNC || req->requestType == READ || req->requestType == LIST || req->requestType == INFO || req->requestType == STREAM){
        handleWrite(req);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }


    return NULL;
}

