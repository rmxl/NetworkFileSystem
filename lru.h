#ifndef LRU_H
#define LRU_H

#include "./commonheaders.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define MAX_LRU_SIZE 10

typedef struct LRUNode LRUNode;

struct LRUNode {
    char path[MAX_PATH_LENGTH];
    int ssid;
    LRUNode *next; 
};

typedef struct {
    int numLRU;
    int maxLRU;
    LRUNode *head;
    LRUNode *tail;
    pthread_mutex_t mutex;
} LRUList;

LRUList* createLRUList(int max);
void enqueueLRU(LRUList **list, const char *path, const int ssid);
void dequeueLRU(LRUList **list);
int retrieveLRU(LRUList **list, const char *path);
void removeFromLRU(LRUList **list, const char *path);
void printLRUList(LRUList **list);
void removefromLRUBySSID(LRUList **list, const int ssid);
#endif
