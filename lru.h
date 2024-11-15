#ifndef LRU_H
#define LRU_H

#include "./commonheaders.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_LRU_SIZE 2

typedef struct LRUNode LRUNode;

struct LRUNode {
    char command[MAX_STRUCT_LENGTH];
    char data[MAX_STRUCT_LENGTH];
    LRUNode *next; 
};

typedef struct {
    int numLRU;
    LRUNode *head;
    LRUNode *tail;
    // pthread_mutex_t mutex;
} LRUList;

extern LRUList *lruCache;

// LRUList* initializeLRUList(LRUList *lruList);
// void enqueueLRU(LRUList **list, const char *command, const char *data);
// void dequeueLRU(LRUList **list);
// const char* retrieveLRU(LRUList **list, const char *command);
// void printLRUList(LRUList **list);

LRUList* initializeLRUList();
void enqueueLRU(LRUList *list, const char *command, const char *data);
void dequeueLRU(LRUList *list);
const char* retrieveLRU(LRUList *list, const char *command);
void printLRUList(LRUList *list);

#endif
