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

void setupConnectionToClient() {
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
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Bind failed");
    close(sockfd);
    exit(EXIT_FAILURE);
}
printf("Bind to port %d successful...\n", SERVER_PORT);

while (1) {
    if (listen(sockfd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }   
    int clientSocketID = findFreeClientSocketIndex();
    if (clientSocketID == -1) {
        perror("No free client sockets");
        continue;  // Continue instead of exiting
    }

    struct sockaddr_in clientaddr;
    int len = sizeof(clientaddr);
    clientSockets[clientSocketID] = accept(sockfd, (struct sockaddr *)&clientaddr, (socklen_t*)&len);

    if (clientSockets[clientSocketID] < 0) {
        perror("Client accept failed");
        continue;
    }
    printf("Client connected on socket %d\n", clientSockets[clientSocketID]);

    request* req = (request*)malloc(sizeof(request));
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
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
    processRequestStruct* prs = (processRequestStruct*)malloc(sizeof(processRequestStruct));
    prs->requestType = req->requestType;
    prs->clientID = clientSocketID;
    memcpy(prs->data, req->data, sizeof(req->data));
    free(req);
    pthread_t requestHandlerThread;
    if (pthread_create(&requestHandlerThread, NULL, processRequests, (void*)prs) != 0) {
        perror("Failed to create request handler thread");
        free(prs);
        close(clientSockets[clientSocketID]);
        clientSockets[clientSocketID] = -1;
    } 
}

    close(sockfd);
    return;
}

int main(int argc, char *argv[]) {
        for(int i = 0; i < MAX_CLIENTS; i++)
        clientSockets[i] = -1;
    printf("Starting naming server...\n");
    // pthread_create(&req_thread, NULL, setupConnectionToClient, NULL);
    // if (pthread_create(&req_thread, NULL, setupConnectionToClient, NULL) != 0) {
    //     perror("Failed to create main setup connection thread");
    //     return EXIT_FAILURE;
    // }
    setupConnectionToClient();

    // pthread_join(req_thread, NULL);  // Wait for the setup thread to finish
    // pthread_join(req_thread, NULL);
    return 0;
}
