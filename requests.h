#ifndef REQUESTS_H
#define REQUESTS_H


#include "./commonheaders.h"
#include "./trie.h"
#include "./namingserver.h" 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <strings.h>
#include <netdb.h>

void* processRequests(void* args);

// void addAccessiblePaths(char* path, int serverIndex);

void createFile(char* path, char* name);
#endif
