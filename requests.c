
#include "./namingserver.h"

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

void connectToSS(StorageServer ss, int ssSocket){
    // connect to the storage server
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss.ssPort);
    serv_addr.sin_addr.s_addr = inet_addr(ss.ssIP);    

    for(int i = 0 ; i < MAX_SERVERS ; i++){
        if(storageServersList[i]->status == -1){
            strcpy(storageServersList[i]->ssIP, ss.ssIP);
            storageServersList[i]->ssPort = ss.ssPort;
            storageServersList[i]->clientPort = ss.clientPort;
            storageServersList[i]->root = create_trie_node();
            storageServersList[i]->numberOfPaths = 2;
            storageServersList[i]->status = 1;
            currentServerCount++;

            //take the list of accessible paths from the storage server
            int bytes_received = 0;
            printf("Waiting for paths from storage server\n");
            while(1){
                char buffer[MAX_STRUCT_LENGTH];
                memset(buffer, 0, sizeof(buffer));
                bytes_received = recv(ssSocket, buffer, sizeof(buffer), 0);
                if(bytes_received <= 0){
                    break;
                }
                char* token = strtok(buffer, " ");
                while(token != NULL){
                    insert_path(storageServersList[i]->root, token, i);
                    storageServersList[i]->numberOfPaths++;
                    token = strtok(NULL, " ");
                }
            }
            printf("Received paths from storage server\n");
            break;
        }
    }    

    close(ssSocket);   
    return;
}

void handleWrite(processRequestStruct* req){
    // char command[MAX_STRUCT_LENGTH];
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
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }   

    //send ip and port of ss to client for them to directly connect
    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    snprintf(data, MAX_STRUCT_LENGTH, "%s %d", ss->ssIP, ss->clientPort);
    printf("Data: %s\n", data);
    sendMessageToClient(clientSockets[req->clientID], ACK, data);
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
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
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
        copy_paths(storageServersList[i]->root, paths);
        strcat(data, paths);
        strcat(data, "\n");
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    sendMessageToClient(clientSockets[req->clientID], ACK, data);
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
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }

    // check if the file/folder already exists
    char checkIfExists[MAX_PATH_LENGTH];
    memset(checkIfExists, 0, sizeof(checkIfExists));
    sprintf(checkIfExists, "%s/%s", cr.path, cr.name);
    if(search_path(ss->root, checkIfExists) >= 0){
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
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to send request to storage server");

        close(sockfd);
        return;
    }

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

        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder created successfully");
    }
    else if(res.requestType == ACK && (req->requestType == DELETEFILE || req->requestType == DELETEFOLDER)){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        delete_path(ss->root, cr.path);
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
    else if(req->requestType == WRITESYNC || req->requestType == WRITEASYNC || req->requestType == READ || req->requestType == INFO || req->requestType == STREAM){
        if (req->requestType == READ || req->requestType == INFO) {
            // combine the request type and path into a single string
            char command[MAX_STRUCT_LENGTH];
            memset(command, 0, sizeof(command));
            sprintf(command, "%d %s", req->requestType, req->data);
            printf("Command: %s\n", command);

            // create buffer to store data from lru cache
            char buffer[MAX_STRUCT_LENGTH];
            memset(buffer, 0, sizeof(buffer));

            printf("printing LRU cache\n");
            printLRUList(lruCache);
            printf("Retrieving data from LRU cache\n");

            const char *result = retrieveLRU(lruCache, command);
            printf("Result: %s\n", result);
            if (result == NULL) {
                handleWrite(req);
            }
            else {
                // strcpy(buffer, result);
                printf("Data retrieved from LRU cache: %s\n", result);
                // send the data to the client
                int ch = send(clientSockets[req->clientID], result, sizeof(result), 0);
                if (ch < 0) {
                    printf("Failed to send response to client");
                }
            }
        }
        else {
            handleWrite(req);
        }    
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);    
    }
    else if(req->requestType == LIST){
        listAllPaths(req);
        clientSockets[req->clientID] = -1;
        close(clientSockets[req->clientID]);
    }
    

    printf("LRU Cache: \n");
    printLRUList(lruCache);

    return NULL;
}

