#include "./storageserver.h"
// #include "./requests.h"
#include "./commonheaders.h"
#include "namingserver.h"
#include <errno.h>
#define PORT 8083
// int sockfd;
int connect_to_ns(char *ns_ip, int ns_port,char* argv) {
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
    server = gethostbyname(ns_ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1;
    }

    // Set up server address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(ns_port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        int errsv = errno;
        printf("errno: %d\n", errsv);
        close(sockfd);
        return -1;
    }
    StorageServer ss;
    ss.ssSocket = sockfd;
    ss.root = NULL;
    pthread_mutex_init(&ss.writeLock, NULL);
    // ss.ssIP = (char*)malloc(strlen(argv) + 1);
    strcpy(ss.ssIP, argv );
    ss.ssPort = PORT;
    char buffer[sizeof(StorageServer)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &ss, sizeof(StorageServer));
    // printf all the information
    printf("IP: %s\n", ss.ssIP);
    printf("Port: %d\n", ss.ssPort);
    printf("Socket: %d\n", ss.ssSocket);
    printf("Root: %p\n", ss.root);
    // printf("Write Lock: %p\n", ss.writeLock);
    request req;
    memset(&req, 0, sizeof(request));
    req.requestType = INITSS;
    memcpy(req.data, buffer, sizeof(buffer));
    char packet[sizeof(request)];
    memset(packet, 0, sizeof(packet));
    memcpy(packet, &req, sizeof(request));

    printf("packet: %s\n", packet);
    int ch=send(sockfd, packet, sizeof(packet), 0);
    if(ch<0){
        perror("Failed to send storage server information");
        close(sockfd);
        return 1;
    }
    // Send IP, port, and identifier information to the Name Server
   
    // char buffer[1024];
    // recv(sockfd, buffer, sizeof(buffer), 0);

    // // Open the accessible paths file
    // FILE *fp = fopen("./accessiblepaths.txt", "r");
    // if (fp == NULL) {
    //     perror("Error opening file");
    //     close(sockfd);
    //     return -1;
    // }

    // // Read and send the contents of accessiblepaths.txt to the server
    // memset(buffer, 0, sizeof(buffer));
    // while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    //     if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
    //         perror("ERROR sending data");
    //         fclose(fp);
    //         close(sockfd);
    //         return -1;
    //     }
    // }
    // fclose(fp);

    return sockfd;
}

int main(int argc, char *argv[]) {
    // Verify command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <NameServer_IP>\n", argv[0]);
        return 1;
    }

    // Connect to the Name Server
    int ns_socket = connect_to_ns(argv[1],NAME_SERVER_PORT ,argv[1]);
    if (ns_socket == -1) {
        printf("Connection to Name Server failed\n");
        return 1;
    }
    printf("Connection to Name Server successful\n");
    
    // Close the socket after the communication is done
    close(ns_socket);
    return 0;
}
