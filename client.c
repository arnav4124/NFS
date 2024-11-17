#include "./commonheaders.h"
#include "./namingserver.h"
#include "./client.h"

// #define TIMEOUT 1000

// void *listenForWriteAck(void *arg)
// {
//     AckThreadArgs *args = (AckThreadArgs *)arg;
//     int sockfd = args->sockfd;
//     int *ack_received = args->ack_received;
//     request ack_packet;
//     ssize_t bytes_received = recv(sockfd, &ack_packet, sizeof(ack_packet), 0);
//     if (bytes_received <= 0)
//     {
//         if (bytes_received < 0)
//             perror("Error: recv() failed");
//         *ack_received = 0;
//     }
//     else
//     {
//         if (ack_packet.requestType == ACK)
//         {
//             printf("Received acknowledgment from storage server: %s\n", ack_packet.data);
//             *ack_received = 1;
//             printf("ack value updated\n");
//         }
//         else
//         {
//             printf("Received unexpected response type: %d\n", ack_packet.requestType);
//             *ack_received = 0;
//         }
//     }
//     return NULL;
// }

void *listenForWriteAck(void *arg)
{
    AckThreadArgs *args = (AckThreadArgs *)arg;
    int sockfd = args->sockfd;
    int *ack_received = args->ack_received;

    struct timeval timeout2;
    timeout2.tv_sec = 0;  
    timeout2.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout2, sizeof(timeout2)) < 0) {
        perror("Error removing socket timeout2");
        close(sockfd);
        return NULL;
    }

    request ack_packet;
    ssize_t bytes_received = recv(sockfd, &ack_packet, sizeof(ack_packet), 0);
    if (bytes_received <= 0)
    {
        if (bytes_received < 0)
            perror("Error: recv() failed");
        *ack_received = 0;
    }
    else
    {
        if (ack_packet.requestType == ACK)
        {
            printf("\nReceived acknowledgment from storage server: %s\n", ack_packet.data);
            *ack_received = 1;
            printf("Write operation acknowledged by storage server\n");
        }
        else
        {
            printf("\nReceived unexpected response type: %d\n", ack_packet.requestType);
            *ack_received = 0;
        }
    }
    close(sockfd);
    return NULL;
}

char* normalizePath(char* path) {
    if (strcmp(path, ".") == 0) return ".";
    while (*path == '.' || *path == '/') {
        path++;
    }
    char* normalizedPath = (char*)malloc(strlen(path) + 3); 
    if (!normalizedPath) return NULL; 
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
    if (strcmp(operation, "STOP") == 0) return STOP; 
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
        if (!(req.requestType == LIST) && !arg1[0]) {
            printf("Error: Invalid format, missing primary argument.\n");
            continue;
        }
        if ((strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0) && !arg2[0]) {
            printf("Error: Missing second argument for %s operation.\n", operation);
            continue;
        }
        if (arg1[0] && arg2[0] && req.requestType != WRITESYNC && req.requestType != LIST) snprintf(req.data, MAX_STRUCT_LENGTH, "%s %s", arg1, arg2);
        else if (arg1[0] && req.requestType != LIST) snprintf(req.data, MAX_STRUCT_LENGTH, "%s", arg1);
        else req.data[0] = '\0';
        printf("arg1: %s, arg2: %s\n", arg1, arg2);
        printf("1RequestType: %d, Data: %s\n", req.requestType, req.data);


        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("Error creating socket");
            continue;
        }
        struct sockaddr_in ns_addr;
        ns_addr.sin_family = AF_INET;
        ns_addr.sin_port = htons(NAME_SERVER_PORT);
        ns_addr.sin_addr.s_addr = inet_addr(nsip);
        if (connect(sockfd, (struct sockaddr*)&ns_addr, sizeof(ns_addr)) < 0) {
            perror("Error connecting to naming server");
            close(sockfd);
            continue;
        }

        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror("Error sending request to naming server");
            close(sockfd);
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 10; 
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("Error setting socket options");
            close(sockfd);
            continue;
        }

        request ns_ack;
        if (recv(sockfd, &ns_ack, sizeof(ns_ack), 0) < 0) {
            perror("Error receiving response from naming server");
            close(sockfd);
            continue;
        }
        
        if (ns_ack.requestType != ACK) {
            printf("Error: Unexpected response from naming server. RequestType: %d\n", ns_ack.requestType);
            close(sockfd);
            continue;
        }
        printf("Received ACK from naming server. Data: %s\n", ns_ack.data);
        


        request ns_response;
        if (recv(sockfd, &ns_response, sizeof(ns_response), 0) < 0) {
            perror("Error receiving response from naming server");
            close(sockfd);
            // exit(1);
            continue;
        }
        if (req.requestType != WRITEASYNC) close(sockfd);
        printf("Naming Server Response - RequestType: %d, \nData: %s\n", ns_response.requestType, ns_response.data);

        if (ns_response.requestType == ERROR) {
            printf("Error: %s\n", ns_response.data);
            continue;
        }
        printf("2RequestType: %d, Data: %s\n", req.requestType, req.data);

        if (req.requestType == LIST) {
            printf("printed list items above");
            continue;
        }    
        
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

        printf("Connecting to storage server...\n");    

        int ss_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (ss_sockfd < 0) {
            perror("Error creating socket");
            continue;
            // exit(1);
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
        if (setsockopt(ss_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("Error setting socket options");
            close(sockfd);
            continue;
        }
        if (send(ss_sockfd, &req, sizeof(req), 0) < 0)
        {
            perror("Error sending request to storage server");
            close(ss_sockfd);
            // exit(1);
            continue;
        }
        printf("request data: %s", req.data);
        int send_write_flag = 1;
        if (req.requestType == WRITEASYNC || req.requestType == WRITESYNC)
        {
            printf("Enter content to write (type 2 consecutive enters to stop):\n");
            char buffer[10];
            int consecutive_newlines = 0;
            ssize_t bytes_sent;
            while (1)
            {
                if (!fgets(buffer, sizeof(buffer), stdin))
                {
                    perror("Error reading input");
                    break;
                }
                if (buffer[0] == '\n')
                {
                    consecutive_newlines++;
                    if (consecutive_newlines == 1)
                        break; 
                }
                else
                    consecutive_newlines = 0; 
                
                request ss_send;
                ss_send.requestType = req.requestType;
                memset(ss_send.data, 0, sizeof(ss_send.data));
                strncpy(ss_send.data, buffer, sizeof(ss_send.data) - 1);
                bytes_sent = send(ss_sockfd, &ss_send, sizeof(ss_send), 0);
                if (bytes_sent < 0)
                {
                    send_write_flag = 0;
                    perror("Error sending data to storage server");
                    close(ss_sockfd);
                    // exit(EXIT_FAILURE);
                    break;
                }
            }
            if (send_write_flag == 0) continue;
            printf("Sending stop\n");
            request ss_send;
            ss_send.requestType = STOP;
            memset(ss_send.data, 0, sizeof(ss_send.data));
            strncpy(ss_send.data, "STOP", sizeof(ss_send.data) - 1);
            bytes_sent = send(ss_sockfd, &ss_send, sizeof(ss_send), 0);
            if (bytes_sent < 0)
            {
                perror("Error sending STOP to storage server");
                close(ss_sockfd);
                // exit(EXIT_FAILURE);
                continue;
            }
            printf("Write operation complete.\n");
        }
        printf("Request sent to storage server.\n");


        if (req.requestType == STREAM) {
            char buffer[MAX_STRUCT_LENGTH];
            FILE *audio_player = popen("ffplay -autoexit -nodisp -", "w");
            if (audio_player == NULL) {
                perror("Error opening audio player");
                close(sockfd);
                // exit(EXIT_FAILURE);
                continue;
            }
            ssize_t bytes_received;
            while ((bytes_received = recv(ss_sockfd, buffer, MAX_STRUCT_LENGTH, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, audio_player);
                fflush(audio_player);
            }
            if (bytes_received < 0) {
                perror("Error receiving audio data");
                continue;
            }
            else printf("Audio stream ended\n");
            pclose(audio_player);
            close(ss_sockfd);
        }
        else if (req.requestType == READ) {
            request ss_response;
            ssize_t bytes_received;
            printf("Starting READ operation...\n");
            while ((bytes_received = recv(ss_sockfd, &ss_response, sizeof(ss_response), 0)) > 0)
            {
                // buffer[bytes_received] = '\0'; 
                printf("%s", ss_response.data);
            }
            if (bytes_received < 0)
                perror("Error receiving data for READ request");
            else
                printf("READ operation completed, no more data.\n");
            close(ss_sockfd);
        }
        else {
            request ss_response;
            if (recv(ss_sockfd, &ss_response, sizeof(ss_response), 0) < 0) {
                perror("Error receiving response from storage server");
                close(ss_sockfd);
                continue;
            }          
            printf("Storage Server Response - RequestType: %d, Data: %s\n", ss_response.requestType, ss_response.data);
            if (req.requestType == WRITEASYNC)
            {
                int ack_received = 0;
                pthread_t ack_thread;
                AckThreadArgs thread_args = {.sockfd = sockfd, .ack_received = &ack_received};
                pthread_create(&ack_thread, NULL, listenForWriteAck, (void *)&thread_args);
                printf("Write operation sent - acknowledgment will be received in background\n");
                continue;
            }
            close(ss_sockfd);
        }        
    }
    return 0;
}
