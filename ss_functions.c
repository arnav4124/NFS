#include "./storageserver.h"
// #include "./requests.h"
#include "./commonheaders.h"
#include "namingserver.h"
#include <errno.h>

#define PORT 8083
#define CLIENT_PORT 8084
// function to start   and bind the socket to the port for listening to the naming server  
void* NS_listener(void* arg){
      struct sockaddr_in serv_addr;
      int ns_sockfd;
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
    serv_addr.sin_port = htons(SERVER_PORT);
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
        
    }

}