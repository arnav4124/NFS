#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "./commonheaders.h"

struct trie_node
{
    char *key;
    int end;
    int ssid;
    struct trie_node *children[256];    // Total 256 characters possible that can come in any path name
};

typedef struct linked_list_head_struct* linked_list_head;
typedef struct linked_list_node_struct* linked_list_node;

typedef struct linked_list_head_struct {
    int number_of_nodes;
    struct linked_list_node_struct* first;
    struct linked_list_node_struct* last;
} linked_list_head_struct;

typedef struct linked_list_node_struct {
    char* path;
    struct linked_list_node_struct* next;
} linked_list_node_struct;
