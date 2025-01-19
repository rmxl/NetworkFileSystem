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

typedef struct writePathNode {
    char path[MAX_PATH_LENGTH];
    int clientID;
    int portNumber;
    pthread_mutex_t mutex;
    char dest_path[MAX_PATH_LENGTH];
    struct writePathNode* next;
} writePathNode;

extern writePathNode* writePathsLL;
extern pthread_mutex_t writePathsLLMutex;

void* heartbeat(void* arg);
// void addAccessiblePaths(char* path, int serverIndex);

void createFile(char* path, char* name);
#endif
