#ifndef STORAGE_SERVER2_H
#define STORAGE_SERVER2_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include<fts.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#define MAX_CLIENTS 1000

// Function declarations
void* handle_ns_req(void* arg);
void* NS_listener(void* arg);
void* handle_client_req(void* arg);
void* Client_listner(void* arg);

#define MAX_CHUNK_AT_A_TIME 10

typedef struct{
    char* buffer;
    char path[MAX_PATH_LENGTH];
} threadArg;

typedef struct file_dir{
    int isFile;
    char path[MAX_PATH_LENGTH];
    char name[MAX_NAME_LENGTH];
    char data[MAX_STRUCT_LENGTH];
}file_dir;
#endif // STORAGE_SERVER2_H 

