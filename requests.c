#include "./namingserver.h" 
#include "./requests.h"
#include "./commonheaders.h"

// void extractFromData(char* data, int type, void* dest){
//     if(type == 0){
//         // SS
//         //first field is in first 1024 bytes, second field is in next 1024 bytes, etc
//         if(dest == IP){
//             //first field
//             strncpy(dest, data, 1024);
//         }else if(dest == PORT){
//             // second field
//             char temp[1024];
//             strncpy(temp, data + 1024, 1024);
//             *((int*)dest) = atoi(temp);
//         }
//     }    
//     else if(type == 1){
//         // NS
//         //first field is in first 1024 bytes, second field is in next 1024 bytes, etc
//         if(dest == PATH){
//             //first field
//             strncpy(dest, data, 1024);
//         }else if(dest == DATA){
//             // second field
//             strncpy(dest, data + 1024, 1024);
//         }
//     }
// }

void connectToSS(StorageServer ss){
    // connect to the storage server
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
    printf("Connected to storage server %s:%d\n", ss.ssIP, ss.ssPort);
    // storageServersList[currentServerCount]->ssPort = serverPorts;
    storageServersList[currentServerCount]->root = ss.root;
    pthread_mutex_init(&storageServersList[currentServerCount]->writeLock, NULL);
    currentServerCount++;
    printf("Connected to storage server %s:%d\n", ss.ssIP, ss.ssPort);

    send(ssSocket, "ACK", 3, 0);

    char buffer[1024];

    // Read accessible paths from the client
    int bytes_received = recv(ssSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive accessible paths");
        close(ssSocket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Parse and store accessible paths
    char *token = strtok(buffer, ",");
    while (token != NULL) {
        // addAccessiblePaths(token, currentServerCount - 1);
        token = strtok(NULL, ",");
    }

    close(ssSocket);   
    return;
}

void createFile(char* path, char* name){
}

void* processRequests(void* args){
    request* req = (request*)args;
    StorageServer ss;
    printf("Processing request\n");
    memset(&ss, 0, sizeof(StorageServer));
    memcpy(&ss, req->data, sizeof(StorageServer));
    if(req->requestType == INITSS){
        // char ip[32];
        // extractFromData(req->data, IP, ip);
        // int port;
        // extractFromData(req->data, PORT, &port);
        printf("Connecting to storage server %s:%d\n", ss.ssIP, ss.ssPort);
        connectToSS(ss);
    }
    else if(req->requestType == CREATE){
        // create file
        char path[1024];
        // extractFromData(req->data, PATH, path);
        char name[1024];
        // extractFromData(req->data, DATA, name);
        createFile(path, name);
    }

    return NULL;
}

