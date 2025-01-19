#include "./lru.h"

LRUList* createLRUList(int max) {
    LRUList *list = (LRUList *)malloc(sizeof(LRUList));
    if (list == NULL) {
        perror("Error allocating memory for LRUList");
        return NULL;
    }
    list->numLRU = 0;
    list->head = NULL;
    list->tail = NULL;
    list->maxLRU = max;
    list->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    return list;
}

void enqueueLRU(LRUList **list, const char *path, const int ssid) {
    if (list == NULL || *list == NULL) {
        return;
    }
    pthread_mutex_lock(&((*list)->mutex));
    LRUNode *current = (*list)->head;
    LRUNode *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->path, path) == 0) {
            if (prev != NULL) {
                prev->next = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = prev;
                }
                current->next = (*list)->head;
                (*list)->head = current;
            }
            current->ssid = ssid;
            pthread_mutex_unlock(&((*list)->mutex));
            return;
        }
        prev = current;
        current = current->next;
    }
    LRUNode *newNode = (LRUNode *)malloc(sizeof(LRUNode));
    if (newNode == NULL) {
        perror("Error allocating memory for LRUNode");
        pthread_mutex_unlock(&((*list)->mutex));
        return;
    }

    strncpy(newNode->path, path, MAX_PATH_LENGTH - 1);
    newNode->path[MAX_PATH_LENGTH - 1] = '\0'; 
    newNode->ssid = ssid;
    newNode->path[MAX_PATH_LENGTH - 1] = '\0';
    newNode->next = (*list)->head;

    (*list)->head = newNode;
    if ((*list)->tail == NULL) {
        (*list)->tail = newNode;
    }

    (*list)->numLRU++;
    if ((*list)->numLRU > (*list)->maxLRU) {
        dequeueLRU(list);
    }
    pthread_mutex_unlock(&((*list)->mutex));
    return;
}

void dequeueLRU(LRUList **list) {
    if (list == NULL ||  (*list) == NULL ||  (*list)->tail == NULL) {
        return;
    }
    if ((*list)->head == (*list)->tail) {
        free((*list)->tail);
        (*list)->head = NULL;
        (*list)->tail = NULL;
    } else {
        LRUNode *current = (*list)->head;
        while (current->next != (*list)->tail) {
            current = current->next;
        }
        free((*list)->tail);
        current->next = NULL;
        (*list)->tail = current;
    }
    (*list)->numLRU--;
    return;
}

void removeFromLRU(LRUList **list, const char *path) {
    if (list == NULL || (*list) == NULL) {
        return;
    }
    pthread_mutex_lock(&((*list)->mutex));
    LRUNode *current = (*list)->head;
    LRUNode *prev = NULL;
    while (current != NULL) {
        if (strcmp(current->path, path) == 0) {
            if (prev != NULL) {
                prev->next = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = prev;
                }
                free(current);
            } else {
                (*list)->head = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = NULL;
                }
                free(current);
            }
            (*list)->numLRU--;
            pthread_mutex_unlock(&((*list)->mutex));
            return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&((*list)->mutex));
    return;
}
void removefromLRUBySSID(LRUList **list, const int ssid) {
    if (list == NULL || (*list) == NULL) {
        return;
    }
    pthread_mutex_lock(&((*list)->mutex));
    LRUNode *current = (*list)->head;
    LRUNode *prev = NULL;
    while (current != NULL) {
        if (current->ssid == ssid) {
            // printf("Removing %s from LRU SSid %d\n", current->path,current->ssid);
            if (prev != NULL) {
                prev->next = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = prev;
                }
                free(current);
            } else {
                (*list)->head = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = NULL;
                }
                free(current);
            }
            (*list)->numLRU--;
            // pthread_mutex_unlock(&((*list)->mutex));
            // return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&((*list)->mutex));
    return;
}

int retrieveLRU(LRUList **list, const char *path) {
    if (list == NULL || (*list) == NULL) { 
        return -1;
    }
    pthread_mutex_lock (&((*list)->mutex));
    LRUNode *current = (*list)->head;
    if(current == NULL) {
        pthread_mutex_unlock(&((*list)->mutex));
        return -1;
    }
    LRUNode *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->path, path) == 0) {
            if (prev != NULL) {
                prev->next = current->next;
                if (current == (*list)->tail) {
                    (*list)->tail = prev;
                }
                current->next = (*list)->head;
                (*list)->head = current;
            }
            pthread_mutex_unlock(&((*list)->mutex));
            return current->ssid;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&((*list)->mutex));
    return -1;
}

void printLRUList(LRUList **list) {
    if (list == NULL || (*list) == NULL) {
        return;
    }

    LRUNode *current = (*list)->head;
    printf("LRU Cache: ");
    while (current != NULL) {
        printf("[%s: %d]", current->path, current->ssid);
        current = current->next;
        if (current != NULL) {
            printf(" -> ");
        }
    }
    printf("\n");
}
