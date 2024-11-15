#include "./commonheaders.h"
#include "./namingserver.h"
#include "./client.h"

#define TIMEOUT 1000

void* listenForWriteAck(void* arg) {
    AckThreadArgs* args = (AckThreadArgs*)arg; 
    int ss_sockfd = args->ss_sockfd;
    int* ack_received = args->ack_received;

    char buffer[MAX_STRUCT_LENGTH];
    memset(buffer, 0, sizeof(buffer));
    struct timeval timeout;
    timeout.tv_sec = 5;  
    timeout.tv_usec = 0;
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(ss_sockfd, &read_fds);
    
    int retval = select(ss_sockfd + 1, &read_fds, NULL, NULL, &timeout);
    if (retval == -1) {
        perror("Error: select() failed");
        *ack_received = 0; 
    } 
    else if (retval == 0) {
        printf("Error: Timeout reached while waiting for acknowledgment.\n");
        *ack_received = 0; 
    } 
    else {
        ssize_t bytes_received = recv(ss_sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            perror("Error: recv() failed or connection closed");
            *ack_received = 0; 
        } 
        else {
            buffer[bytes_received] = '\0'; 
            printf("Received acknowledgment from storage server: %s\n", buffer);
            *ack_received = 1; 
            printf("ack value updated\n");
        }
    }
    return NULL;
}



char* normalizePath(char* path) {
    if (strcmp(path, ".") == 0) return ".";
    while (*path == '.' || *path == '/') {
        path++;
    }
    char* normalizedPath = (char*)malloc(strlen(path) + 3); // 2 for "./" and 1 for '\0'
    if (!normalizedPath) return NULL;   
    // strcpy(normalizedPath, "./");
    // strcat(normalizedPath, path);
    strcpy(normalizedPath, path);
    return normalizedPath;
}

requestType getRequestType(const char* operation) {
    if (strcmp(operation, "READ") == 0) return READ;
    if (strcmp(operation, "WRITESYNC") == 0) return WRITESYNC;
    if (strcmp(operation, "WRITEASYNC") == 0) return WRITEASYNC;
    if (strcmp(operation, "CREATEFILE") == 0) return CREATEFILE; 
    if (strcmp(operation, "CREATEFOLDER") == 0) return CREATEFOLDER;
    if (strcmp(operation, "DELETEFILE") == 0) return DELETEFILE; 
    if (strcmp(operation, "DELETEFOLDER") == 0) return DELETEFOLDER;
    if (strcmp(operation, "COPYFILE") == 0) return COPYFILE;
    if (strcmp(operation, "COPYFOLDER") == 0) return COPYFOLDER;
    if (strcmp(operation, "LIST") == 0) return LIST;
    if (strcmp(operation, "INFO") == 0) return INFO;
    if (strcmp(operation, "STREAM") == 0) return STREAM;
    if (strcmp(operation, "EXIT") == 0) return INITSS;
    if (strcmp(operation, "ACK") == 0) return ACK;
    if (strcmp(operation, "ERROR") == 0) return ERROR;
    if (strcmp(operation, "LRU") == 0) return LRU;
    if (strcmp(operation, "ASYNC_ACK") == 0) return ASYNC_ACK;
    return INITSS; 
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <naming server ip>\n", argv[0]);
        exit(1);
    }
    char* nsip = argv[1];
    // printf("Naming server IP: %s\n", nsip);
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
        // printf("Input: %s\n", input);
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
        else if (strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0 || strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "CREATEFILE") == 0) {
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
        if ((strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0) && !arg2[0]) {
            printf("Error: Missing second argument for %s operation.\n", operation);
            continue;
        }
        if (arg1[0] && arg2[0] && req.requestType != WRITESYNC) snprintf(req.data, MAX_STRUCT_LENGTH, "%s %s", arg1, arg2);
        else if (arg1[0]) snprintf(req.data, MAX_STRUCT_LENGTH, "%s", arg1);
        else req.data[0] = '\0';
        printf("arg1: %s, arg2: %s\n", arg1, arg2);
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




        // fd_set read_fds;
        // FD_ZERO(&read_fds);
        // FD_SET(sockfd, &read_fds);        
        // struct timeval timeout;
        // timeout.tv_sec = 10; 
        // timeout.tv_usec = 0;
        // int retval = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        // if (retval == -1) {
        //     perror("Error: select() failed");
        //     close(sockfd);
        //     exit(1);
        // } 
        // else if (retval == 0) {
        //     printf("Error: Timeout reached while waiting for initial ACK from naming server.\n");
        //     close(sockfd);
        //     exit(1);
        // } 
        // else {
        //     request ns_response;
        //     ssize_t bytes_received = recv(sockfd, &ns_response, sizeof(ns_response), 0);
        //     if (bytes_received <= 0) {
        //         perror("Error: recv() failed or connection closed by naming server");
        //         close(sockfd);
        //         exit(1);
        //     }
        //     if (ns_response.requestType == ACK) {
        //         printf("Received ACK from naming server. Data: %s\n", ns_response.data);
        //         if (send(sockfd, &req, sizeof(req), 0) < 0) {
        //             perror("Error sending request to naming server");
        //             close(sockfd);
        //             exit(1);
        //         }
        //     } 
        //     else {
        //         printf("Error: Unexpected response from naming server. RequestType: %d\n", ns_response.requestType);
        //         close(sockfd);
        //         exit(1);
        //     }
        // }



        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request to naming server");
            close(sockfd);
            exit(1);
        }

        request ns_ack;
        if (recv(sockfd, &ns_ack, sizeof(ns_ack), 0) < 0) {
            perror("Error receiving response from naming server");
            close(sockfd);
            exit(1);
        }

        
        if (ns_ack.requestType != ACK) {
            printf("Error: Unexpected response from naming server. RequestType: %d\n", ns_ack.requestType);
            close(sockfd);
            exit(1);
        }
        printf("Received ACK from naming server. Data: %s\n", ns_ack.data);
        


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
        if (ns_response.requestType == LRU) {
            printf("Data received from LRU cache - %s\n", ns_response.data);
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
        fflush(stdout);

        


        printf("operation: %s\n", operation);
        if (strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "DELETEFILE") == 0 || strcmp(operation, "DELETEFOLDER") == 0 || strcmp(operation, "COPY") == 0) {
            continue;
        }

        printf("RequestType: %d, Data: %s\n", req.requestType, req.data);
        printf("Connecting to storage server...\n");    

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
            // exit(1);
            continue;
        }
        if (send(ss_sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request to storage server");
            close(ss_sockfd);
            exit(1);
        }
        printf("Request sent to storage server.\n");

        request ss_response;
        if (recv(ss_sockfd, &ss_response, sizeof(ss_response), 0) < 0) {
            perror("Error receiving response from storage server");
            close(ss_sockfd);
            exit(1);
        }
        printf("Storage Server Response - RequestType: %d, Data: %s\n", ss_response.requestType, ss_response.data);
       
        if (req.requestType == WRITESYNC || ss_response.requestType == ASYNC_ACK) {
            int ack_received = 0;
            pthread_t ack_thread;
            AckThreadArgs thread_args = { .ss_sockfd = ss_sockfd, .ack_received = &ack_received };
            pthread_create(&ack_thread, NULL, listenForWriteAck, (void*)&thread_args);
            pthread_join(ack_thread, NULL);
            if (ack_received) printf("Success: WRITE_SYNC operation completed successfully.\n");
            else printf("Error: Timeout reached waiting for WRITE_SYNC acknowledgment.\n");    
            close(ss_sockfd);
            continue;
        }
       
        if (req.requestType == STREAM) {
            char buffer[MAX_STRUCT_LENGTH];
            FILE *audio_player = popen("ffplay -autoexit -nodisp -", "w");
            if (audio_player == NULL) {
                perror("Error opening audio player");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
            ssize_t bytes_received;
            while ((bytes_received = recv(ss_sockfd, buffer, MAX_STRUCT_LENGTH, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, audio_player);
                fflush(audio_player);
            }
            if (bytes_received < 0) perror("Error receiving audio data");
            else printf("Audio stream ended\n");
            pclose(audio_player);
            close(ss_sockfd);
        }

        close(ss_sockfd);
    }
    return 0;
}
