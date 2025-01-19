#ifndef TRIE_H
#define TRIE_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "./commonheaders.h"

// Trie node structure
typedef struct Node {
    char *path_fragment;
    int is_terminal;
    int id;
    struct Node *children[256]; // 256 ASCII characters for possible keys
} TrieNode;

// Linked list node and list structures
typedef struct PathNode {
    char *path;
    struct PathNode *next;
} PathNode;

typedef struct PathList {
    int size;
    struct PathNode *head;
    struct PathNode *tail;
} PathList;

// Function prototypes
TrieNode *initialize_node();
int add_path(TrieNode *root, const char *path, int id);
int find_path(TrieNode *root, const char *path);
int remove_path(TrieNode *root, const char *path);
PathList *initialize_path_list();
PathNode *initialize_path_node(const char *path);
void append_to_list(PathList *list, const char *path);
void collect_paths(PathList *list, TrieNode *root);
void release_path_list(PathList *list);
void store_paths_in_buffer(TrieNode *root, char *buffer);

#endif 
