#include "./namingserver.h"

int connectToSS(int sockfd) {
    // Listen for connections
    if (listen(sockfd, 10) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Accept connection
    int ssSocket = accept(sockfd, (struct sockaddr *)NULL, NULL);
    if (ssSocket < 0) {
        perror("Server accept failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server accepted the client...\n");

    char buffer[1024];

    // Read IP and port from the client
    ssize_t bytes_received = recv(ssSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive IP and port");
        close(ssSocket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Parse IP and port
    char *token = strtok(buffer, " ");
    if (token == NULL) {
        fprintf(stderr, "Invalid data format for IP and port\n");
        close(ssSocket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    storageServersList[currentServerCount] = malloc(sizeof(StorageServer));
    storageServersList[currentServerCount]->ssIP = malloc(strlen(token) + 1);
    strcpy(storageServersList[currentServerCount]->ssIP, token);

    token = strtok(NULL, " ");
    if (token == NULL) {
        fprintf(stderr, "Invalid data format for port\n");
        close(ssSocket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    storageServersList[currentServerCount]->ssPort = atoi(token);
    storageServersList[currentServerCount]->ssSocket = ssSocket;
    storageServersList[currentServerCount]->ssPort = serverPorts;
    currentServerCount++;

    send(ssSocket, "ACK", 3, 0);

    // Read accessible paths from the client
    bytes_received = recv(ssSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        perror("Failed to receive accessible paths");
        close(ssSocket);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received string

    // Parse and store accessible paths
    char *tokn = strtok(buffer, ",");
    while (tokn != NULL) {
        accessiblePaths[currentPathCount] = malloc(strlen(tokn) + 1);
        strcpy(accessiblePaths[currentPathCount], tokn);
        currentPathCount++;
        printf("Path: %s\n", tokn);
        tokn = strtok(NULL, ",");
    }
    return ssSocket;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
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
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(SERVER_PORT);
    
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Bind to port %d successful...\n", SERVER_PORT);

    // Connect to storage server
    int ssSocket = connectToSS(sockfd);

    // Close sockets when done
    close(ssSocket);
    close(sockfd);
    return 0;
}
