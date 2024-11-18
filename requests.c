#include "./namingserver.h"

void insertWritePath(writePathNode** writePathsLL, int clientID, char* path){
    //insert in linked list at head
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* newNode = (writePathNode*)malloc(sizeof(writePathNode));
    newNode->clientID = clientID;
    strcpy(newNode->path, path);
    pthread_mutex_init(&newNode->mutex, NULL);
    pthread_mutex_lock(&newNode->mutex);
    newNode->next = *writePathsLL;
    *writePathsLL = newNode;
    pthread_mutex_unlock(&writePathsLLMutex);
    return;
}

int tryWritePath(writePathNode** writePathsLL, char* path){
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* temp = *writePathsLL;
    while(temp != NULL){
        if(strcmp(temp->path, path) == 0 || strstr(temp->path, path) != NULL){
            //path already being written
            // printf("Path already being written\n");
            pthread_mutex_unlock(&writePathsLLMutex);
            return 0;
        }
        temp = temp->next;
    }
    //path not found   
    pthread_mutex_unlock(&writePathsLLMutex);
    return 1;
}

int removeWritePath(writePathNode** writePathsLL, char* path){
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* temp = *writePathsLL;
    writePathNode* prev = NULL;
    while(temp != NULL){
        if(strcmp(temp->path, path) == 0){
            if(prev == NULL){
                *writePathsLL = temp->next;
            }
            else{
                prev->next = temp->next;
            }
            int id = temp->clientID;
            free(temp);
            pthread_mutex_unlock(&writePathsLLMutex);
            return id;
        }
        prev = temp;
        temp = temp->next;
    }
    pthread_mutex_unlock(&writePathsLLMutex);
    return -1;
}


int retrievePathIndex(char* path){
    //search in cache first
    int index = retrieveLRU(&lruCache, path);
    if(index >= 0){
        printf("Path found in cache\n");
        return index;
    }

    for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);
        if(find_path(storageServersList[i]->root, path) >= 0){
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            return i;
        }
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    return -1;
}

void sendMessageToClient(int clientSocket, requestType type, char* data){
    request req;
    memset(&req, 0, sizeof(request));
    req.requestType = type;
    strcpy(req.data, data);

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &req, sizeof(request));

    int ch = send(clientSocket, buffer, sizeof(request), 0);
    if(ch < 0){
        printf("Failed to send response to client");
    }
    // struct sockaddr_in clientAddr;
    // socklen_t addrLen = sizeof(clientAddr);
    // if (getpeername(clientSocket, (struct sockaddr *)&clientAddr, &addrLen) == -1)
    // {
    //     perror("Failed to get client address");
    //     return;
    // }
    // char clientIP[INET_ADDRSTRLEN];
    // inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    // int clientPort = ntohs(clientAddr.sin_port);
    // logsentto("Client", clientIP, clientPort, buffer);

    return;
}

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
    serv_addr.sin_port = htons(port);

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

void connectToSS(StorageServer ss, int ssSocket){
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss.ssPort);
    serv_addr.sin_addr.s_addr = inet_addr(ss.ssIP);    

    for(int i = 0 ; i < MAX_SERVERS ; i++){
        if(storageServersList[i]->status == -1){
            strcpy(storageServersList[i]->ssIP, ss.ssIP);
            storageServersList[i]->ssPort = ss.ssPort;
            storageServersList[i]->clientPort = ss.clientPort;
            storageServersList[i]->root = initialize_node();
            storageServersList[i]->numberOfPaths = 0;
            storageServersList[i]->status = 1;
            currentServerCount++;

            //take the list of accessible paths from the storage server
            printf("Waiting for paths from storage server\n");
            char buffer[MAX_STRUCT_LENGTH];
            memset(buffer, 0, sizeof(buffer));

            int size  = recv(ssSocket, buffer, sizeof(buffer), 0);

            if(size <= 0){
                printf("Failed to receive paths from storage server\n");
            }
            logrecvfrom("Storage Server", ss.ssIP, ss.ssPort, buffer);

            char* token = strtok(buffer, " ");
            pthread_mutex_lock(&storageServersList[i]->mutex);
            while(token != NULL){
                add_path(storageServersList[i]->root, token, i);
                storageServersList[i]->numberOfPaths++;
                token = strtok(NULL, " ");
            }
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            printf("Received paths from storage server\n");
            break;
        }    
    }
    close(ssSocket);   
    return;
}

void handleCopyingFolders(processRequestStruct* req){
    char path_source[MAX_PATH_LENGTH] = {0}, path_dest[MAX_PATH_LENGTH] = {0};
    char* token = strtok(req->data, " ");
    strcpy(path_source, token);
    token = strtok(NULL, " ");
    strcpy(path_dest, token);
    int ssid1 = retrievePathIndex(path_source);
    int ssid2 = retrievePathIndex(path_dest);
    if(ssid1 == -1 || ssid2 == -1){
        printf("Invalid paths provided\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Invalid paths provided");
        return;
    }

    //check if the folder path provided is a substring of any paths being written to. If yes, return error
    writePathNode* temp = writePathsLL;
    while(temp != NULL){
        if(strstr(temp->path, path_source) != NULL){
            printf("Path is already being written to\n");
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
            return;
        }
        temp = temp->next;
    }

    // insert the path being written to so that no other client can write to it
    insertWritePath(&writePathsLL, req->clientID, path_source);


    StorageServer* ss_source = storageServersList[ssid1], *ss_dest = storageServersList[ssid2];

    request req1;
    memset(&req1, 0, sizeof(req1));
    sprintf(req1.data, "%s %d %s %s",ss_dest->ssIP, ss_dest->clientPort, path_source, path_dest);
    req1.requestType = COPYFOLDER;

    int fd_source = connectTo(ss_source->ssPort, ss_source->ssIP);

    char data[MAX_STRUCT_LENGTH] = {0};
    memcpy(data, &req1, MAX_STRUCT_LENGTH);
    int c1 = send(fd_source, data, MAX_STRUCT_LENGTH, 0);
    if(c1 < 0) {
        //later
    }
    logsentto("Storage Server", ss_dest->ssIP, ss_dest->ssPort, data);

    printf("Copy request sent.\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, "Copy started");
    return;
}

void handleCopyingFiles(processRequestStruct* req){
    char path_source[MAX_PATH_LENGTH] = {0}, path_dest[MAX_PATH_LENGTH] = {0};
    char* token = strtok(req->data, " ");
    strcpy(path_source, token);
    token = strtok(NULL, " ");
    strcpy(path_dest, token);
    printf("Source: %s ", path_source);
    printf("Dest: %s\n", path_dest);
    int ssid1 = retrievePathIndex(path_source);
    int ssid2 = retrievePathIndex(path_dest);


    if(ssid1 == -1 || ssid2 == -1){
        printf("Invalid paths provided\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Invalid paths provided");
        return;
    }

    //check if file is already being written to
    if(tryWritePath(&writePathsLL, path_source) == 0){
        printf("Path is already being written to\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }
    
    insertWritePath(&writePathsLL, req->clientID, path_source);

    StorageServer* ss_source = storageServersList[ssid1], *ss_dest = storageServersList[ssid2];

    request req1;
    memset(&req1, 0, sizeof(req1));
    sprintf(req1.data, "%s %d %s %s",ss_dest->ssIP, ss_dest->clientPort, path_source, path_dest);
    req1.requestType = COPYFILE;

    int fd_source = connectTo(ss_source->ssPort, ss_source->ssIP);

    char data[MAX_STRUCT_LENGTH] = {0};
    memcpy(data, &req1, MAX_STRUCT_LENGTH);
    int c1 = send(fd_source, data, MAX_STRUCT_LENGTH, 0);
    if(c1 < 0) {
        //later
    }
    logsentto("Storage Server", ss_dest->ssIP, ss_dest->ssPort, data);

    printf("Copy request sent.\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, "Copy request sent");

}

void handleClientSSRequests(processRequestStruct* req){
    client_request cr;
    memset(&cr, 0, sizeof(client_request));
    //req->data has path
    strcpy(cr.path, req->data);
    printf("Path: %s\n", cr.path);   
    

    // check if path is already being written to
    if(tryWritePath(&writePathsLL, req->data) == 0){
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }

    if(req->requestType == WRITEASYNC || req->requestType == WRITESYNC){
        insertWritePath(&writePathsLL, req->clientID, req->data);
    }

    //find storage server with the path
    StorageServer* ss;
    int checkPathIfPresent = retrievePathIndex(cr.path);
    if(checkPathIfPresent == -1){
        printf("Path not found\n");
        printf("%s\n", cr.path);
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }   

    ss = storageServersList[checkPathIfPresent];

    //send ip and port of ss to client for them to directly connect
    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    snprintf(data, MAX_STRUCT_LENGTH, "%s %d", ss->ssIP, ss->clientPort);

    enqueueLRU(&lruCache, cr.path, checkPathIfPresent);

    printf("Sending IP and Port to client\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, data);
    return;
}

void handleDelete(processRequestStruct* req){
    char data[MAX_PATH_LENGTH];
    memset(data, 0, sizeof(data));
    strcpy(data, req->data);

    // check if path is already being written to
    if(tryWritePath(&writePathsLL, req->data) == 0){
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }

    insertWritePath(&writePathsLL, req->clientID, req->data);

    //find storage server with the path
    int checkPathIfPresent = retrievePathIndex(data);
    if(checkPathIfPresent == -1){
        printf("Path not found\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }   

    StorageServer* ss = storageServersList[checkPathIfPresent];

    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to connect to storage server");

        close(sockfd);
        return;
    }

    request reqq;
    memset(&reqq, 0, sizeof(request));
    reqq.requestType = req->requestType;
    memcpy(reqq.data, req->data, sizeof(client_request));

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &reqq, sizeof(request));

    int ch = send(sockfd, buffer, sizeof(request), 0);
    if(ch < 0){
        printf("Failed to send request to storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to send request to storage server");

        close(sockfd);
        return;
    }
    logsentto("Storage Server", ss->ssIP, ss->ssPort, buffer);

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);

    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server\n");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to receive response from storage server");

        close(sockfd);
        return;
    }
    logrecvfrom("Storage Server", ss->ssIP, ss->ssPort, response);

    request res;
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    if(res.requestType == ACK){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        remove_path(ss->root, data);
        ss->numberOfPaths--;
        pthread_mutex_unlock(&ss->mutex);

        removeFromLRU(&lruCache, data);
        
        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder deleted successfully");
    }
    else{
        printf("Error : %s\n", res.data);
        //sending this to client

        sendMessageToClient(clientSockets[req->clientID], ERROR, res.data);
    }

    close(sockfd);
    return;
}

void listAllPaths(processRequestStruct* req){
    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);
        char paths[MAX_STRUCT_LENGTH];
        memset(paths, 0, sizeof(paths));
        store_paths_in_buffer(storageServersList[i]->root, paths);
        strcat(data, paths);
        strcat(data, "\n");
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    sendMessageToClient(clientSockets[req->clientID], ACK, data);
    return;
}

// handles the Create File/Folder request, takes the request struct as input, searches for the path in the storage servers and sends the request to the appropriate storage server
void handleCreate(processRequestStruct* req){
    char data[MAX_PATH_LENGTH], path[MAX_PATH_LENGTH];
    memset(data, 0, sizeof(data));
    char* token = strtok(req->data, " ");
    strcpy(data, token);
    strcpy(path, data);
    token = strtok(NULL, " ");
    strcat(data, "/");
    strcat(data, token);

    StorageServer* ss;
    int checkPathIfPresent;

    if(strcmp(path, ".") == 0){
        // choose the storage server with least number of paths
        int min = 100000000;
        int index = -1;

        for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
            pthread_mutex_lock(&storageServersList[i]->mutex);
            if(storageServersList[i]->numberOfPaths < min){
                min = storageServersList[i]->numberOfPaths;
                index = i;
            }
            pthread_mutex_unlock(&storageServersList[i]->mutex);
        }

        if(index == -1){
            printf("No storage server available\n");
            //sending this to client
            sendMessageToClient(clientSockets[req->clientID], ERROR, "No storage server available");
            return;
        }
        ss = storageServersList[index];
        checkPathIfPresent = index;
    }
    else {
        checkPathIfPresent = retrievePathIndex(path);
        if(checkPathIfPresent == -1){
            printf("Path not found\n");
            //sending this to client
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
            return;
        }
        ss = storageServersList[checkPathIfPresent];
    }

    // check if the file/folder already exists
    if(find_path(ss->root, data) >= 0){
        printf("File/Folder already exists\n");
        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "File/Folder already exists");
        return;
    }

    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to connect to storage server");

        close(sockfd);
        return;
    }


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
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to send request to storage server");

        close(sockfd);
        return;
    }
    logsentto("Storage Server", ss->ssIP, ss->ssPort, buffer);

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);
    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to receive response from storage server");

        close(sockfd);
        return;
    }
    logrecvfrom("Storage Server", ss->ssIP, ss->ssPort, response);

    request res; 
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    if(res.requestType == ACK && (req->requestType == CREATEFILE || req->requestType == CREATEFOLDER)){
        printf("File/Folder created successfully\n");
        // add path to the trie
        pthread_mutex_lock(&ss->mutex);
        add_path(ss->root, data, checkPathIfPresent);
        ss->numberOfPaths++;
        pthread_mutex_unlock(&ss->mutex);

        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder created successfully");
    }
    else if(res.requestType == ACK && (req->requestType == DELETEFILE || req->requestType == DELETEFOLDER)){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        remove_path(ss->root, data);
        pthread_mutex_unlock(&ss->mutex);

        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder deleted successfully");
    }
    else if(res.requestType == ERROR){
        printf("Error : %s\n", res.data);
        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, res.data);
    }

    close(sockfd);
    return;
}

// Thread function to process requests
void* processRequests(void* args){
    processRequestStruct* req = (processRequestStruct*)args;
    printf("Processing request\n");

    sendMessageToClient(clientSockets[req->clientID], ACK, "Request received");

    if(req->requestType == INITSS){
        StorageServer ss;
        memset(&ss, 0, sizeof(StorageServer));
        memcpy(&ss, req->data, sizeof(StorageServer));
        printf("Connecting to storage server %s:%d\n", ss.ssIP, ss.ssPort);
        connectToSS(ss, clientSockets[req->clientID]);
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
    else if(req->requestType == WRITESYNC ||  req->requestType == READ || req->requestType == INFO || req->requestType == STREAM){
        handleClientSSRequests(req); 
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    else if(req->requestType == WRITEASYNC ){
        handleClientSSRequests(req);  
    }
    else if(req->requestType == LIST){
        listAllPaths(req);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    else if(req->requestType == ACK){
        // unlock the path
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);
        removeWritePath(&writePathsLL, data);
    }
    else if(req->requestType == ASYNC_WRITE_ACK){
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);
        int id = removeWritePath(&writePathsLL, data);
        sendMessageToClient(clientSockets[id], ACK, "Data was written succesfully.");
        clientSockets[id] = -1;
        close(clientSockets[id]);
    }
    else if(req->requestType == COPYFOLDER){
        handleCopyingFolders(req);
    }
    else if(req->requestType == COPYFILE){
        handleCopyingFiles(req);
    }
    else if(req->requestType == REGISTER_PATH){
        // insert the path in the trie
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);
        
        char* token = strtok(data, " ");

        int port = atoi(token);
        char* path = strtok(NULL, " ");
        printf("Registering path %s with port %d\n", path, port);

        //find the ss with the port
        for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
            if(storageServersList[i]->ssPort == port){
                pthread_mutex_lock(&storageServersList[i]->mutex);
                add_path(storageServersList[i]->root, path, i);
                storageServersList[i]->numberOfPaths++;
                pthread_mutex_unlock(&storageServersList[i]->mutex);
                break;
            }
        }        
    }
    else if(req->requestType == REGISTER_PATH_STOP){
        int id = removeWritePath(&writePathsLL, req->data);
        printf("Path %s is no longer being written to\n", req->data);
        sendMessageToClient(clientSockets[id], ACK, "Process Completed.");
        clientSockets[id] = -1;
        close(clientSockets[id]);
    }
    return NULL;
}


