#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SERVERS 10
#define MAX_PATHS 1024

#define SERVER_PORT 8082

typedef struct {
    char* ssIP;
    int ssPort;
    int ssSocket;
} StorageServer;

int currentServerCount = 0;
StorageServer* storageServersList[MAX_SERVERS];

int currentPathCount = 0;
char* accessiblePaths[MAX_PATHS];

int sockfd;

int serverPorts = 8100;
