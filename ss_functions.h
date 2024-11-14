#ifndef STORAGE_SERVER2_H
#define STORAGE_SERVER2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8083
#define CLIENT_PORT 8084
#define MAX_CLIENTS 10
// #define SERVER_PORT 8083

// Enumeration for request types
// typedef enum {
//     READ,
//     WRITE,
//     DELETE
// } RequestType;

// Structure for request
// typedef struct {
//     RequestType requestType;
//     char data[1024]; // Adjust size based on your requirements
// } request;

// Function declarations
void* handle_ns_req(void* arg);
void* NS_listener(void* arg);
void* handle_client_req(void* arg);
void* Client_listner(void* arg);

#endif // STORAGE_SERVER_H
