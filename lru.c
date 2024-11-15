#include "./lru.h"

// LRUList* initializeLRUList(LRUList *list) {
//     list = (LRUList *)malloc(sizeof(LRUList));
//     if (list == NULL) {
//         perror("Error allocating memory for LRUList");
//         exit(1);
//     }
//     (list)->numLRU = 0;
//     (list)->head = NULL;
//     (list)->tail = NULL;
//     return list;
//     // pthread_mutex_init(&(*list)->mutex, NULL);
// }

// void enqueueLRU(LRUList **list, const char *command, const char *data) {
//     // pthread_mutex_lock(&list->mutex);
//     LRUNode *current = (*list)->head, *prev = NULL;
//     while (current != NULL) {
//         if (strcmp(current->command, command) == 0) {
//             // Move to head
//             if (prev != NULL) {
//                 prev->next = current->next;
//                 if (current == (*list)->tail) {
//                     (*list)->tail = prev;
//                 }
//                 current->next = (*list)->head;
//                 (*list)->head = current;
//             }
//             strncpy(current->data, data, MAX_STRUCT_LENGTH - 1);
//             current->data[MAX_STRUCT_LENGTH - 1] = '\0';
//             // pthread_mutex_unlock(&list->mutex);
//             return;
//         }
//         prev = current;
//         current = current->next;
//     }

//     // Add a new node
//     LRUNode *newNode = (LRUNode *)malloc(sizeof(LRUNode));
//     strncpy(newNode->command, command, MAX_STRUCT_LENGTH - 1);
//     newNode->command[MAX_STRUCT_LENGTH - 1] = '\0';
//     strncpy(newNode->data, data, MAX_STRUCT_LENGTH - 1);
//     newNode->data[MAX_STRUCT_LENGTH - 1] = '\0';
//     newNode->next = (*list)->head;

//     (*list)->head = newNode;
//     if ((*list)->tail == NULL) {
//         (*list)->tail = newNode;
//     }

//     (*list)->numLRU++;
//     if ((*list)->numLRU > MAX_LRU_SIZE) {
//         dequeueLRU(list);
//     }

//     // pthread_mutex_unlock(&list->mutex);
// }







// void enqueueLRU(LRUList **list, const char *command, const char *data) {
//     if (list == NULL || *list == NULL) {
//         perror("Invalid LRU list");
//         exit(1);
//     }

//     LRUNode *current = (*list)->head, *prev = NULL;
//     while (current != NULL) {
//         if (strcmp(current->command, command) == 0) {
//             // Move to head
//             if (prev != NULL) {
//                 prev->next = current->next;
//                 if (current == (*list)->tail) {
//                     (*list)->tail = prev;
//                 }
//                 current->next = (*list)->head;
//                 (*list)->head = current;
//             }
//             strncpy(current->data, data, MAX_STRUCT_LENGTH - 1);
//             current->data[MAX_STRUCT_LENGTH - 1] = '\0';
//             return;
//         }
//         prev = current;
//         current = current->next;
//     }

//     // Add a new node
//     LRUNode *newNode = (LRUNode *)malloc(sizeof(LRUNode));
//     if (newNode == NULL) {
//         perror("Error allocating memory for LRUNode");
//         exit(1);
//     }
//     strncpy(newNode->command, command, MAX_STRUCT_LENGTH - 1);
//     newNode->command[MAX_STRUCT_LENGTH - 1] = '\0';
//     strncpy(newNode->data, data, MAX_STRUCT_LENGTH - 1);
//     newNode->data[MAX_STRUCT_LENGTH - 1] = '\0';
//     newNode->next = (*list)->head;

//     (*list)->head = newNode;
//     if ((*list)->tail == NULL) {
//         (*list)->tail = newNode;
//     }

//     (*list)->numLRU++;
//     if ((*list)->numLRU > MAX_LRU_SIZE) {
//         dequeueLRU(list);
//     }
// }


// void dequeueLRU(LRUList **list) {
//     // pthread_mutex_lock(&list->mutex);

//     if ((*list)->tail == NULL) {
//         // pthread_mutex_unlock(&list->mutex);
//         return;
//     }

//     if ((*list)->head == (*list)->tail) {
//         free((*list)->tail);
//         (*list)->head = NULL;
//         (*list)->tail = NULL;
//     } else {
//         LRUNode *current = (*list)->head;
//         while (current->next != (*list)->tail) {
//             current = current->next;
//         }
//         free((*list)->tail);
//         current->next = NULL;
//         (*list)->tail = current;
//     }
//     (*list)->numLRU--;

//     // pthread_mutex_unlock(&list->mutex);
// }

// const char* retrieveLRU(LRUList **list, const char *command) {
//     // pthread_mutex_lock(&list->mutex);

//     LRUNode *current = (*list)->head, *prev = NULL;
//     while (current != NULL) {
//         if (strcmp(current->command, command) == 0) {
//             if (prev != NULL) {
//                 prev->next = current->next;
//                 if (current == (*list)->tail) {
//                     (*list)->tail = prev;
//                 }
//                 current->next = (*list)->head;
//                 (*list)->head = current;
//             }
//             // pthread_mutex_unlock(&list->mutex);
//             return current->data;
//         }
//         prev = current;
//         current = current->next;
//     }

//     // pthread_mutex_unlock(&list->mutex);
//     return NULL;
// }

// void printLRUList(LRUList **list) {
//     // pthread_mutex_lock(&list->mutex);

//     LRUNode *current = (*list)->head;
//     printf("LRU Cache: ");
//     while (current != NULL) {
//         printf("[%s: %s]", current->command, current->data);
//         current = current->next;
//         if (current != NULL) {
//             printf(" -> ");
//         }
//     }
//     printf("\n");

//     // pthread_mutex_unlock(&list->mutex);
// }



LRUList* initializeLRUList() {
    LRUList *list = (LRUList *)malloc(sizeof(LRUList));
    if (list == NULL) {
        perror("Error allocating memory for LRUList");
        return NULL;
    }
    list->numLRU = 0;
    list->head = NULL;
    list->tail = NULL;
    // pthread_mutex_init(&list->mutex, NULL);
    return list;
}

void enqueueLRU(LRUList *list, const char *command, const char *data) {
    if (list == NULL) {
        return;
    }

    // pthread_mutex_lock(&list->mutex);

    LRUNode *current = list->head;
    LRUNode *prev = NULL;
    
    // Check if command already exists
    while (current != NULL) {
        if (strcmp(current->command, command) == 0) {
            // Move to head if not already there
            if (prev != NULL) {
                prev->next = current->next;
                if (current == list->tail) {
                    list->tail = prev;
                }
                current->next = list->head;
                list->head = current;
            }
            strncpy(current->data, data, MAX_STRUCT_LENGTH - 1);
            current->data[MAX_STRUCT_LENGTH - 1] = '\0';
            // pthread_mutex_unlock(&list->mutex);
            return;
        }
        prev = current;
        current = current->next;
    }

    // Create new node
    LRUNode *newNode = (LRUNode *)malloc(sizeof(LRUNode));
    if (newNode == NULL) {
        // pthread_mutex_unlock(&list->mutex);
        return;
    }

    strncpy(newNode->command, command, MAX_STRUCT_LENGTH - 1);
    newNode->command[MAX_STRUCT_LENGTH - 1] = '\0';
    strncpy(newNode->data, data, MAX_STRUCT_LENGTH - 1);
    newNode->data[MAX_STRUCT_LENGTH - 1] = '\0';
    newNode->next = list->head;

    list->head = newNode;
    if (list->tail == NULL) {
        list->tail = newNode;
    }

    list->numLRU++;
    if (list->numLRU > MAX_LRU_SIZE) {
        dequeueLRU(list);
    }

    // pthread_mutex_unlock(&list->mutex);
}

void dequeueLRU(LRUList *list) {
    if (list == NULL || list->tail == NULL) {
        return;
    }

    // pthread_mutex_lock(&list->mutex);

    if (list->head == list->tail) {
        free(list->tail);
        list->head = NULL;
        list->tail = NULL;
    } else {
        LRUNode *current = list->head;
        while (current->next != list->tail) {
            current = current->next;
        }
        free(list->tail);
        current->next = NULL;
        list->tail = current;
    }
    list->numLRU--;

    // pthread_mutex_unlock(&list->mutex);
}

const char* retrieveLRU(LRUList *list, const char *command) {
    if (list == NULL) {
        return NULL;
    }

    // pthread_mutex_lock(&list->mutex);

    LRUNode *current = list->head;
    LRUNode *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->command, command) == 0) {
            // Move to head if not already there
            if (prev != NULL) {
                prev->next = current->next;
                if (current == list->tail) {
                    list->tail = prev;
                }
                current->next = list->head;
                list->head = current;
            }
            // pthread_mutex_unlock(&list->mutex);
            return current->data;
        }
        prev = current;
        current = current->next;
    }

    // pthread_mutex_unlock(&list->mutex);
    return NULL;
}

void printLRUList(LRUList *list) {
    if (list == NULL) {
        return;
    }

    // pthread_mutex_lock(&list->mutex);

    LRUNode *current = list->head;
    printf("LRU Cache: ");
    while (current != NULL) {
        printf("[%s: %s]", current->command, current->data);
        current = current->next;
        if (current != NULL) {
            printf(" -> ");
        }
    }
    printf("\n");

    // pthread_mutex_unlock(&list->mutex);
}
