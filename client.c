#include "./commonheaders.h"
#include "./client.h"

#define TIMEOUT 60

int ack_received = 0;

// void* listenForAck(void* args) {
//     int sockfd = *((int*)args);
//     struct timeval timeout;
//     timeout.tv_sec = TIMEOUT;  
//     timeout.tv_usec = 0;    
//     fd_set readfds;
//     FD_ZERO(&readfds);
//     FD_SET(sockfd, &readfds);
//     if (select(sockfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
//         request response;
//         if (recv(sockfd, &response, sizeof(response), 0) > 0) {
//             if (response.requestType == ACK && strcmp(response.data, "2") == 0) {
//                 ack_received = 1;
//             }
//         }
//     }
//     return NULL;
// }

char* normalizePath(char* path) {
    while (*path == '.' || *path == '/') {
        path++;
    }
    return path;
}

requestType getRequestType(const char* operation) {
    if (strcmp(operation, "READ") == 0) return READ;
    if (strcmp(operation, "WRITESYNC") == 0) return WRITESYNC;
    if (strcmp(operation, "WRITEASYNC") == 0) return WRITEASYNC;
    // if (strcmp(operation, "APPEND") == 0) return APPEND;
    if (strcmp(operation, "CREATEFILE") == 0) return CREATEFILE; 
    if (strcmp(operation, "CREATEFOLDER") == 0) return CREATEFOLDER;
    if (strcmp(operation, "DELETE") == 0) return DELETEFILE; 
    if (strcmp(operation, "DELETEFOLDER") == 0) return DELETEFOLDER;
    if (strcmp(operation, "COPY") == 0) return COPY;
    if (strcmp(operation, "LIST") == 0) return LIST;
    if (strcmp(operation, "INFO") == 0) return INFO;
    if (strcmp(operation, "STREAM") == 0) return STREAM;
    if (strcmp(operation, "EXIT") == 0) return INITSS;
    if (strcmp(operation, "ACK") == 0) return ACK;
    if (strcmp(operation, "ERROR") == 0) return ERROR;
    return INITSS; 
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <naming server ip>\n", argv[0]);
        exit(1);
    }
    char* nsip = argv[1];
    printf("Naming server IP: %s\n", nsip);
    char input[MAX_NAME_LENGTH];
    char operation[MAX_IPOP_LENGTH], arg1[MAX_PATH_LENGTH], arg2[MAX_PATH_LENGTH];
    printf("...............Client starting.............\n");

    while (1) {
        printf("Enter Command: ");
        if (!fgets(input, sizeof(input), stdin)) {
            fprintf(stderr, "Error reading input.\n");
            continue;
        }
        input[strcspn(input, "\n")] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';
        char* token = strtok(input, " ");
        if (token == NULL) {
            printf("Error: No operation provided.\n");
            continue;
        }
        strncpy(operation, token, MAX_IPOP_LENGTH);
        operation[MAX_IPOP_LENGTH - 1] = '\0';
        token = strtok(NULL, " ");
        if (token != NULL) {
            strncpy(arg1, token, MAX_PATH_LENGTH);
            arg1[MAX_PATH_LENGTH - 1] = '\0';
            strcpy(arg1, normalizePath(arg1));
        }
        request req;
        req.requestType = getRequestType(operation);
        if (strcmp(operation, "WRITE") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(arg2, token, MAX_PATH_LENGTH);
                arg2[MAX_PATH_LENGTH - 1] = '\0';
                strcpy(arg2, normalizePath(arg2));
                if (strcmp(arg2, "--SYNC") == 0) req.requestType = WRITESYNC;
                else {
                    printf("Error: Unrecognized flag for WRITE operation.\n");
                    continue;
                }                
            }
            else req.requestType = WRITEASYNC;
        }
        else if (strcmp(operation, "COPY") == 0 || strcmp(operation, "CREATE") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(arg2, token, MAX_PATH_LENGTH);
                arg2[MAX_PATH_LENGTH - 1] = '\0';
            }
        }
        if (!arg1[0]) {
            printf("Error: Invalid format, missing primary argument.\n");
            continue;
        }
        if ((strcmp(operation, "CREATE") == 0 || strcmp(operation, "COPY") == 0) && !arg2[0]) {
            printf("Error: Missing second argument for %s operation.\n", operation);
            continue;
        }
        if (arg1[0] && arg2[0]) snprintf(req.data, MAX_STRUCT_LENGTH, "%s %s", arg1, arg2);
        else if (arg1[0]) snprintf(req.data, MAX_STRUCT_LENGTH, "%s", arg1);
        else req.data[0] = '\0';
        printf("1RequestType: %d, Data: %s\n", req.requestType, req.data);




        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error creating socket");
            exit(1);
        }
        struct sockaddr_in ns_addr;
        ns_addr.sin_family = AF_INET;
        ns_addr.sin_port = htons(NAME_SERVER_PORT);
        ns_addr.sin_addr.s_addr = inet_addr(nsip);
        if (connect(sockfd, (struct sockaddr*)&ns_addr, sizeof(ns_addr)) < 0) {
            perror("Error connecting to naming server");
            close(sockfd);
            exit(1);
        }
        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request to naming server");
            close(sockfd);
            exit(1);
        }
        char response[MAX_DATA_LENGTH];
        
        // if (recv(sockfd, response, sizeof(response), 0) < 0) {
        //     perror("Error receiving response from naming server");
        //     close(sockfd);
        //     exit(1);
        // }
        // close(sockfd);
        request ns_response;
        if (recv(sockfd, &ns_response, sizeof(ns_response), 0) < 0) {
            perror("Error receiving response from naming server");
            close(sockfd);
            exit(1);
        }
        close(sockfd);
        printf("Naming Server Response - RequestType: %d, \nData: %s\n", ns_response.requestType, ns_response.data);

        if (ns_response.requestType == ERROR) {
            printf("Error: %s\n", ns_response.data);
            continue;
        }
        printf("2RequestType: %d, Data: %s\n", req.requestType, req.data);

        if (req.requestType == WRITEASYNC || req.requestType == WRITESYNC) {
            printf("Enter content to write (type 2 consecutive enters to stop):\n");
            char content[MAX_STRUCT_LENGTH];
            content[0] = '\0';
            while (1) {
                char line[MAX_STRUCT_LENGTH];
                if (!fgets(line, sizeof(line), stdin)) {
                    fprintf(stderr, "Error reading input.\n");
                    break;
                }
                if (line[0] == '\n') break; 
                strncat(content, line, sizeof(content) - strlen(content) - 1);
            }
            strcat(req.data, "\n");
            strcat(req.data, content);
            printf("content: %s", content);

        }
        printf("3RequestType: %d, Data: %s\n", req.requestType, req.data);

        
        char storage_server_ip[MAX_IPOP_LENGTH], storage_server_port[MAX_IPOP_LENGTH];
        memset(storage_server_ip, 0, sizeof(storage_server_ip));
        memset(storage_server_port, 0, sizeof(storage_server_port));
        if (sscanf(ns_response.data, "%s %s", storage_server_ip, storage_server_port) != 2) {
            printf("Error: Invalid response from naming server.\n");
            continue;
        }
        printf("Storage Server IP: %s, Port: %s\n", storage_server_ip, storage_server_port);

        




        int ss_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (ss_sockfd < 0) {
            perror("Error creating socket");
            exit(1);
        }
        struct sockaddr_in ss_addr;
        ss_addr.sin_family = AF_INET;
        ss_addr.sin_port = htons(atoi(storage_server_port));
        ss_addr.sin_addr.s_addr = inet_addr(storage_server_ip);
        if (connect(ss_sockfd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) < 0) {
            perror("Error connecting to storage server");
            close(ss_sockfd);
            exit(1);
        }
        if (send(ss_sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request to storage server");
            close(ss_sockfd);
            exit(1);
        }


        // if (req.requestType == WRITESYNC) {
        //     // Receive initial acknowledgment from storage server
        //     request ss_response;
        //     if (recv(sockfd, &ss_response, sizeof(ss_response), 0) < 0) {
        //         perror("Error receiving response from storage server");
        //         close(sockfd);
        //         exit(1);
        //     }

        //     if (ss_response.requestType == ACK && strcmp(ss_response.data, "1") == 0) {
        //         printf("Storage server acknowledged write start.\n");

        //         // Start a background thread to listen for ACK type 2
        //         pthread_t ack_thread;
        //         pthread_create(&ack_thread, NULL, listenForAck, (void*)&sockfd);

        //         // Wait for either ACK type 2 or timeout
        //         sleep(TIMEOUT);

        //         if (ack_received) {
        //             printf("Success: WRITE_SYNC operation completed successfully.\n");
        //         } else {
        //             printf("Error: Timeout reached waiting for WRITE_SYNC acknowledgment.\n");
        //         }

        //         // Close the socket and join thread
        //         close(sockfd);
        //         pthread_join(ack_thread, NULL);
        //     }
        //     else {
        //         printf("Error: Did not receive expected ACK from storage server.\n");
        //     }
        // }
        // else {
            request ss_response;
            if (recv(ss_sockfd, &ss_response, sizeof(ss_response), 0) < 0) {
                perror("Error receiving response from storage server");
                close(ss_sockfd);
                exit(1);
            }
            close(ss_sockfd);
            printf("Storage Server Response - RequestType: %d, Data: %s\n", ss_response.requestType, ss_response.data);
        //     close(sockfd);
        // }        

    }

    return 0;
}
