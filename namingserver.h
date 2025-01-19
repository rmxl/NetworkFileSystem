
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

#define MAX_CLIENTS 1000

extern int clientSockets[MAX_CLIENTS];
extern pthread_mutex_t clientSocketsLock;

typedef struct {
  char ssIP[20];
  int ssPort;
  TrieNode* root;
  int clientPort;
  int status;
  int numberOfPaths;
  int backupPort1;
  int backupPort2;
  char backupFolder[MAX_PATH_LENGTH];
  pthread_mutex_t mutex;
  int is_BackedUp;
} StorageServer;

extern int currentServerCount;
extern StorageServer* storageServersList[MAX_SERVERS];

extern int sockfdMaster;

extern int serverPorts ;

extern LRUList* lruCache;
#endif
