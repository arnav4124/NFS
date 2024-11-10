#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/pthreadtypes.h>
#include <arpa/inet.h>

#include "./commonheaders.h"
#include "./requests.h"
#include "./trie.h"

#define MAX_SERVERS 10
#define MAX_PATHS 1024

#define SERVER_PORT 8082

#define MAX_CLIENTS 10

int clientSockets[MAX_CLIENTS];

typedef struct {
    char* ssIP;
    int ssPort;
    int ssSocket;
    struct trie_node* root;
    pthread_mutex_t writeLock;
} StorageServer;

int currentServerCount = 0;
StorageServer* storageServersList[MAX_SERVERS];

int sockfd;

int serverPorts = 8100;
