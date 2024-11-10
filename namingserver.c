#include "./namingserver.h"

int findFreeClientSocketIndex() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientSockets[i] == -1) {
            return i;
        }
    }
    return -1;
}

void* setupConnectionToClient(void* args){
    int sockfd;
    struct sockaddr_in servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket successfully created...\n");

    // Prepare server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(SERVER_PORT);
    
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Bind to port %d successful...\n", SERVER_PORT);

    if(listen(sockfd, MAX_CLIENTS) < 0){
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while(1){
        int clientSocketID = findFreeClientSocketIndex();
        if(clientSocketID == -1){
            perror("No free client sockets");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);
        clientSockets[clientSocketID] = accept(sockfd, (struct sockaddr *)&clientaddr, &len);

        if(clientSockets[clientSocketID] < 0){
            perror("Client accept failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        request* req = (request*)malloc(sizeof(request));
        ssize_t bytes_received = recv(clientSockets[clientSocketID], req, sizeof(request), 0);

        pthread_t requestHandlerThread;

        pthread_create(&requestHandlerThread, NULL, processRequests, (void*)req);
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char *argv[]) {
    
    // int sockfd;
    // struct sockaddr_in servaddr;
    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (sockfd == -1) {
    //     perror("Socket creation failed");
    //     exit(EXIT_FAILURE);
    // }
    // printf("Socket successfully created...\n");

    // // Prepare server address
    // bzero(&servaddr, sizeof(servaddr));
    // servaddr.sin_family = AF_INET;
    // servaddr.sin_addr.s_addr = INADDR_ANY;
    // servaddr.sin_port = htons(SERVER_PORT);
    
    // // Bind the socket
    // if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    //     perror("Bind failed");
    //     close(sockfd);
    //     exit(EXIT_FAILURE);
    // }
    // printf("Bind to port %d successful...\n", SERVER_PORT);

    // // Connect to storage server
    // int ssSocket = connectToSS(sockfd);

    // // Close sockets when done
    // close(ssSocket);
    // close(sockfd);
    return 0;
}
