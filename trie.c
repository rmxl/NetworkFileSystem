#include "./trie.h"

// Initialize a new trie node
TrieNode *initialize_node() {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (!node) {
        printf("Failed to allocate memory for trie node: %s\n", strerror(errno));
        return NULL;
    }
    node->path_fragment = NULL;
    node->is_terminal = 0;
    node->id = -1;
    for (int i = 0; i < 256; ++i) {
        node->children[i] = NULL;
    }
    return node;
}

// Insert a path into the trie
int add_path(TrieNode *root, const char *path, int id) {
    printf("Adding path: %s\n", path);
    TrieNode *current = root;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        int index = (unsigned char)path[i];
        if (!current->children[index]) {
            current->children[index] = initialize_node();
            if (!current->children[index]) {
                printf("Failed to initialize node for path: %s\n", path);
                return 0;
            }
        }
        current = current->children[index];
    }
    current->path_fragment = strdup(path);
    current->is_terminal = 1;
    current->id = id;
    return 1;
}

// Search for a path in the trie
int find_path(TrieNode *root, const char *path) {
    TrieNode *current = root;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        int index = (unsigned char)path[i];
        if (!current->children[index]) {
            return -1;
        }
        current = current->children[index];
    }
    return current->is_terminal ? current->id : -1;
}

// Remove a path from the trie (lazy deletion)
int remove_path(TrieNode *root, const char *path) {
    TrieNode *current = root;
    for (size_t i = 0; path[i] != '\0'; ++i) {
        int index = (unsigned char)path[i];
        if (!current->children[index]) {
            return 0;
        }
        current = current->children[index];
    }
    current->path_fragment = NULL;
    current->is_terminal = 0;
    current->id = -1;
    return 1;
}

// Copy all paths into a buffer
void store_paths_in_buffer(TrieNode *root, char *buffer) {
    if (!root) return;
    if (root->is_terminal) {
        strcat(buffer, root->path_fragment);
        strcat(buffer, "\n");
    }
    for (int i = 0; i < 256; ++i) {
        if (root->children[i]) {
            store_paths_in_buffer(root->children[i], buffer);
        }
    }
}

// Initialize a new path list
PathList *initialize_path_list() {
    PathList *list = (PathList *)malloc(sizeof(PathList));
    if (!list) {
        printf("Failed to allocate memory for path list: %s\n", strerror(errno));
        return NULL;
    }
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    return list;
}

// Initialize a new path node
PathNode *initialize_path_node(const char *path) {
    PathNode *node = (PathNode *)malloc(sizeof(PathNode));
    if (!node) {
        printf("Failed to allocate memory for path node: %s\n", strerror(errno));
        return NULL;
    }
    node->path = strdup(path);
    node->next = NULL;
    return node;
}

// Append a path to the list
void append_to_list(PathList *list, const char *path) {
    PathNode *node = initialize_path_node(path);
    if (!node) return;
    if (!list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
}

// Collect paths recursively into a path list
void collect_paths(PathList *list, TrieNode *root) {
    if (!root) return;
    if (root->is_terminal) {
        append_to_list(list, root->path_fragment);
    }
    for (int i = 0; i < 256; ++i) {
        if (root->children[i]) {
            collect_paths(list, root->children[i]);
        }
    }
}

// Free the memory allocated for the path list
void release_path_list(PathList *list) {
    PathNode *current = list->head;
    while (current) {
        PathNode *temp = current;
        free(temp->path);
        current = current->next;
        free(temp);
    }
    free(list);
}
