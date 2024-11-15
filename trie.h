#ifndef TRIE_H
#define TRIE_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "./commonheaders.h"

#define RED(string) "\033[1;31m" string "\033[0m"

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
#endif

// Function prototypes
struct trie_node *create_trie_node();
int insert_path(struct trie_node *root, char *key,int ssid);
int search_path(struct trie_node *root, char *key);
int delete_path(struct trie_node *root, char *key);
void print_paths(struct trie_node *root);
linked_list_head create_linked_list_head();
linked_list_node create_linked_list_node(char* path);
void insert_in_linked_list(linked_list_head linked_list, char* path);
void add_paths(linked_list_head ll, struct trie_node *root);
void free_linked_list(linked_list_head linked_list);
void copy_paths(struct trie_node *root, char* buffer);
