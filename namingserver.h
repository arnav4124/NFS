
#ifndef NAMINGSERVER_H
#define NAMINGSERVER_H

#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/pthreadtypes.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "./requests.h"
#include "./trie.h"
#include "./lru.h"

#define MAX_SERVERS 10
#define MAX_PATHS 1024

#define SERVER_PORT 8082

#define MAX_CLIENTS 10

extern int clientSockets[MAX_CLIENTS];
// extern LRUList* lruCache;

typedef struct {
  char ssIP[20];
  int ssPort;
  struct trie_node* root;
  int clientPort;
  int status;
  int numberOfPaths;
  pthread_mutex_t mutex;
} StorageServer;

extern   int currentServerCount ;
extern StorageServer* storageServersList[MAX_SERVERS];

extern int sockfd;

extern int serverPorts ;
#endif
