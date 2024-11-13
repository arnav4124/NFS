#include "./namingserver.h"
#include "./requests.h"
#include "./commonheaders.h"
int serverPorts = 8082;
StorageServer* storageServersList[MAX_SERVERS];
int currentServerCount = 0;
int sockfd;
int clientSockets[MAX_CLIENTS];
int findFreeClientSocketIndex() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientSockets[i] == -1) {
            return i;
        }
    }
    return -1;
}
void* setupConnectionToClient(void* args) {
    printf("Starting to set up connections...\n");

    // Set up the socket once, before the loop
   struct sockaddr_in servaddr;
sockfd = socket(AF_INET, SOCK_STREAM, 0);
if (sockfd == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
}
printf("Socket successfully created...\n");

bzero(&servaddr, sizeof(servaddr));
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr = INADDR_ANY;
servaddr.sin_port = htons(SERVER_PORT);

if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
}
printf("Bind to port %d successful...\n", SERVER_PORT);

if (listen(sockfd, MAX_CLIENTS) < 0) {
    perror("Listen failed");
    close(sockfd);
    exit(EXIT_FAILURE);
}
    while (1) {
        int clientSocketID = findFreeClientSocketIndex();
        if (clientSocketID == -1) {
            perror("No free client sockets");
            continue;  // Continue instead of exiting
        }

        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);
        clientSockets[clientSocketID] = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

        if (clientSockets[clientSocketID] < 0) {
            perror("Client accept failed");
            continue;
        }
        printf("Client connected on socket %d\n", clientSockets[clientSocketID]);

        request* req = (request*)malloc(sizeof(request));
        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(buffer));
        // ssize_t bytes_received = recv(clientSockets[clientSocketID], req, sizeof(request), 0);
        ssize_t bytes_received = recv(clientSockets[clientSocketID], buffer, sizeof(request), 0);
        if (bytes_received <= 0) {
            perror("Failed to receive request");
            close(clientSockets[clientSocketID]);
            clientSockets[clientSocketID] = -1;
            free(req);
            continue;
        }
        memcpy(req, buffer, sizeof(request));
        printf("Received request from client on socket %d\n", clientSockets[clientSocketID]);
        pthread_t requestHandlerThread;
        if (pthread_create(&requestHandlerThread, NULL, processRequests, (void*)req) != 0) {
            perror("Failed to create request handler thread");
            free(req);
            close(clientSockets[clientSocketID]);
            clientSockets[clientSocketID] = -1;
        } 
        pthread_join(requestHandlerThread, NULL);
    }

    close(sockfd);
    return NULL;
}

// void* setupConnectionToClient(void* args){  
// //   while(1){
//     printf("Socket successfully created...\n");
//     // while(1){
//     struct sockaddr_in servaddr;
//    sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sockfd == -1) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // Prepare server address
//     bzero(&servaddr, sizeof(servaddr));
//     servaddr.sin_family = AF_INET;
//     servaddr.sin_addr.s_addr = INADDR_ANY;
//     servaddr.sin_port = htons(SERVER_PORT);
    
//     // Bind the socket
//     if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
//         perror("Bind failed");
//         close(sockfd);
//         exit(EXIT_FAILURE);
//     }
//     printf("Bind to port %d successful...\n", SERVER_PORT);

//     if(listen(sockfd, MAX_CLIENTS) < 0){
//         perror("Listen failed");
//         close(sockfd);
//         exit(EXIT_FAILURE);
//     }

//     while(1){
//         int clientSocketID = findFreeClientSocketIndex();
//         if(clientSocketID == -1){
//             perror("No free client sockets");
//             close(sockfd);
//             exit(EXIT_FAILURE);
//         }
        
//         struct sockaddr_in clientaddr;
//         int len = sizeof(clientaddr);
//         clientSockets[clientSocketID] = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

//         if(clientSockets[clientSocketID] < 0){
//             perror("Client accept failed");
//             close(sockfd);
//             exit(EXIT_FAILURE);
//         }

//         request* req = (request*)malloc(sizeof(request));
//         ssize_t bytes_received = recv(clientSockets[clientSocketID], req, sizeof(request), 0);

//         pthread_t requestHandlerThread;

//         pthread_create(&requestHandlerThread, NULL, processRequests, (void*)req);
//     }

//     close(sockfd);
//     // }
// //   }
//     return NULL;

// }

int main(int argc, char *argv[]) {
        for(int i = 0; i < MAX_CLIENTS; i++)
        clientSockets[i] = -1;
    pthread_t req_thread;
    printf("Starting naming server...\n");
    // pthread_create(&req_thread, NULL, setupConnectionToClient, NULL);
    if (pthread_create(&req_thread, NULL, setupConnectionToClient, NULL) != 0) {
        perror("Failed to create main setup connection thread");
        return EXIT_FAILURE;
    }

    pthread_join(req_thread, NULL);  // Wait for the setup thread to finish
    // pthread_join(req_thread, NULL);
    return 0;
}
