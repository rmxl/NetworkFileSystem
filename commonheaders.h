#ifndef COMMONHEADERS_H
#define COMMONHEADERS_H

#define _BSD_SOURCE

#include <stdio.h>

#define NAME_SERVER_PORT 8082

typedef enum {
   READ,
   WRITESYNC,
   WRITEASYNC,
   CREATEFILE,
   CREATEFOLDER,
   DELETEFILE,
   DELETEFOLDER,
   COPYFILE,
   COPYFOLDER, 
   PASTE,
   PASTEFILE,
   PASTEFOLDER,
   COPYFILESAME,
   COPYFOLDERSAME,   
   LIST,
   INFO,
   STREAM,
   INITSS,
   ACK,
   ERROR,
   WRITE_ACK,
   REGISTER_PATH,
   REGISTER_PATH_STOP_FILE,
   REGISTER_PATH_STOP_FOLDER,
   HEARTBEAT,
   IAMALIVE,
   STOP,
} requestType;

#define MAX_IPOP_LENGTH 20
#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 1024
#define MAX_DATA_LENGTH 4096
#define MAX_STRUCT_LENGTH 5000
#define STREAM_BUFFER_SIZE 1000000
#define MAX_WRITE_BUFFER_SIZE 10000000

typedef struct {
    requestType requestType;
    char data[MAX_STRUCT_LENGTH];
} request;

typedef struct {
    char name[MAX_NAME_LENGTH];
    char path[MAX_PATH_LENGTH];
} client_request;

typedef struct{
  requestType requestType;
  char data[MAX_STRUCT_LENGTH];
  int clientID;
} processRequestStruct;

void logrecvfrom(char *from, char *ip, int port, char *buffer);
void logsentto(char *to, char *ip, int port, char *buffer);
void logerror(char *error);

#endif
