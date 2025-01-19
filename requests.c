#include "./namingserver.h"

void removeFromClientSocket(int clientSocket){
    pthread_mutex_lock(&clientSocketsLock);
    close(clientSockets[clientSocket]);
    clientSockets[clientSocket] = -1;
    pthread_mutex_unlock(&clientSocketsLock);
}
int getSSIDFromPort(int port){
    for(int i = 0; i < currentServerCount; i++){
        if(storageServersList[i]->ssPort == port){
            return i;
        }
    }
    return -1;
}
// Send data to a port
int connectTo(int port, char* ip){
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  

    // Get server information
    server = gethostbyname(ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1;
    }

    // Set up server address structure
    // bzero((char *)&serv_addr, sizeof(serv_addr));
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        int errsv = errno;
        printf("errno: %d\n", errsv);
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void insertWritePath(writePathNode** writePathsLL, int clientID, char* path, int dest_port, char* dest_path){
    //insert in linked list at head
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* newNode = (writePathNode*)malloc(sizeof(writePathNode));
    newNode->clientID = clientID;
    strcpy(newNode->path, path);
    strcpy(newNode->dest_path, dest_path);
    pthread_mutex_init(&newNode->mutex, NULL);
    pthread_mutex_lock(&newNode->mutex);
    newNode->next = *writePathsLL;
    *writePathsLL = newNode;
    newNode->portNumber = dest_port;
    printf("Path inserted in writePathsLL\n");
    pthread_mutex_unlock(&writePathsLLMutex);
    return;
}

int tryWritePath(writePathNode** writePathsLL, char* path){
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* temp = *writePathsLL;
    while(temp != NULL){
        if(strcmp(temp->path, path) == 0 || strstr(temp->path, path) != NULL){
            //path already being written
            // printf("Path already being written\n");
            pthread_mutex_unlock(&writePathsLLMutex);
            return 0;
        }
        temp = temp->next;
    }
    //path not found   
    pthread_mutex_unlock(&writePathsLLMutex);
    return 1;
}

int removeWritePath(writePathNode** writePathsLL, char* path){
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* temp = *writePathsLL;
    writePathNode* prev = NULL;
    while(temp != NULL){
        if(strcmp(temp->path, path) == 0){
            if(prev == NULL){
                *writePathsLL = temp->next;
            }
            else{
                prev->next = temp->next;
            }
            int id = temp->clientID;
            free(temp);
            pthread_mutex_unlock(&writePathsLLMutex);
            return id;
        }
        prev = temp;
        temp = temp->next;
    }
    pthread_mutex_unlock(&writePathsLLMutex);
    return -1;
}

writePathNode* findWritePath(writePathNode** writePathsLL, char* path){
    pthread_mutex_lock(&writePathsLLMutex);
    writePathNode* temp = *writePathsLL;
    while(temp != NULL){
        if(strcmp(temp->path, path) == 0){
            pthread_mutex_unlock(&writePathsLLMutex);
            return temp;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&writePathsLLMutex);
    return NULL;
}

void backup(int port, int ssid, char* dest_path, requestType type){

    if(strstr(dest_path, storageServersList[ssid]->backupFolder) != NULL) return;
    printf("Backing up %s\n", dest_path);

    if((storageServersList[ssid])->is_BackedUp){
        //send the file to the backup storage servers

        StorageServer* backup1 = storageServersList[getSSIDFromPort((storageServersList[ssid])->backupPort1)];
        StorageServer* backup2 = storageServersList[getSSIDFromPort((storageServersList[ssid])->backupPort2)];
        int sockfd1;
        if(backup1->status == 1) {
        if(type==DELETEFILE || type==DELETEFOLDER)
            {
            sockfd1 = connectTo(backup1->ssPort, backup1->ssIP);
            }
            else sockfd1 = connectTo(storageServersList[ssid]->ssPort, storageServersList[ssid]->ssIP);


            if(sockfd1 < 0){
                printf("Failed to connect to backup storage server\n");
                return;
            }
            
            request req1;
            memset(&req1, 0, sizeof(request));
            req1.requestType = type;
            if(type==DELETEFILE || type==DELETEFOLDER)
            {
                strcpy(req1.data, backup1->backupFolder);
                strcat(req1.data, "/");
                strcat(req1.data, dest_path);
                char buff1[sizeof(request)];
                memset(buff1, 0, sizeof(request));
                memcpy(buff1, &req1, sizeof(request));
                int c1 = send(sockfd1, buff1, sizeof(request), 0);
                if(c1 < 0) {
                    //later
                }
            }
            //if the path is dest_path, then the path in the backup will be backup/dest_path
            else{
            char data[MAX_STRUCT_LENGTH] = {0};
            if(type == COPYFOLDER) {
                sprintf(data, "%s %d %s %s", backup1->ssIP, backup1->clientPort, dest_path, backup1->backupFolder);
            }
            
            else{ 
                char data4[MAX_STRUCT_LENGTH] = {0};
                //copy everything from dest_path until the last / to data2
                char* temp2 = strrchr(dest_path, '/');
                strncpy(data4, dest_path, temp2 - dest_path);
                data4[temp2 - dest_path] = '\0';
                sprintf(data, "%s %d %s %s/%s", backup1->ssIP, backup1->clientPort, dest_path, backup1->backupFolder, data4);
            }        
            printf("Data1: %s\n", data);

            
            memcpy(req1.data, data, sizeof(data));
            char buffer[sizeof(request)];
            memset(buffer, 0, sizeof(request));
            memcpy(buffer, &req1, sizeof(request));
            
            int c1 = send(sockfd1, buffer, sizeof(request), 0);
            if(c1 < 0) {
                //later
                printf("Failed to send request to backup storage server\n");
                logerror("Failed to send request to backup storage server");
            }
            logsentto("Storage Server", storageServersList[ssid]->ssIP, storageServersList[ssid]->ssPort, req1.data);
            close(sockfd1);  
            }
        }
        // sleep(1);
        if(backup2->status == 1){
            int sockfd2;
            if(type==DELETEFILE || type==DELETEFOLDER)
            {
                sockfd2 = connectTo(backup2->ssPort, backup2->ssIP);
            }
            else  sockfd2  = connectTo(storageServersList[ssid]->ssPort, storageServersList[ssid]->ssIP);      

            request req2;
            memset(&req2, 0, sizeof(request));
            req2.requestType = type;        
            if(type==DELETEFILE || type==DELETEFOLDER)
            {
                strcpy(req2.data, backup2->backupFolder);
                strcat(req2.data, "/");
                strcat(req2.data, dest_path);
                char buff2[sizeof(request)];
                memset(buff2, 0, sizeof(request));
                memcpy(buff2, &req2, sizeof(request));
                int c2 = send(sockfd2, buff2, sizeof(request), 0);
                if(c2 < 0) {
                    //later
                }
                return;
            }
            
            char data2[MAX_STRUCT_LENGTH] = {0};
            if(type == COPYFOLDER) {
                sprintf(data2, "%s %d %s %s", backup2->ssIP, backup2->clientPort, dest_path, backup2->backupFolder);
            }
            else {
                char data3[MAX_STRUCT_LENGTH] = {0};
                //copy everything from dest_path until the last / to data2
                char* temp = strrchr(dest_path, '/');
                strncpy(data3, dest_path, temp - dest_path);
                data3[temp - dest_path] = '\0';
                sprintf(data2, "%s %d %s %s/%s", backup2->ssIP, backup2->clientPort, dest_path, backup2->backupFolder, data3);
            }
            printf("Data2: %s\n", data2);

            memcpy(req2.data, data2, sizeof(data2));
            char buffer2[sizeof(request)];
            memset(buffer2, 0, sizeof(request));
            memcpy(buffer2, &req2, sizeof(request));

            int c2 = send(sockfd2, buffer2, sizeof(request), 0);
            if(c2 < 0) {
                //later
                printf("Failed to send request to backup storage server\n");
                logerror("Failed to send request to backup storage server");
            }

            logsentto("Storage Server", storageServersList[ssid]->ssIP, storageServersList[ssid]->ssPort, req2.data);
            close(sockfd2);
        }

    }
    printf("Backed up %s\n", dest_path);
}

int retrievePathIndex(char* path){
    //search in cache first
    int index = retrieveLRU(&lruCache, path);
    if(index >= 0){
        printf("Path found in cache\n");
        return index;
    }

    for(int i = 0; i < currentServerCount && storageServersList[i]->status != -1; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);
        if(find_path(storageServersList[i]->root, path) >= 0){
            if(storageServersList[i]->status == 0){
                // storage server is down
                pthread_mutex_unlock(&storageServersList[i]->mutex);
                return -2;
            }
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            return i;
        }
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }
    return -1;
}

void sendMessageToClient(int clientSocket, requestType type, char* data){
    request req;
    memset(&req, 0, sizeof(request));
    req.requestType = type;
    strcpy(req.data, data);

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &req, sizeof(request));

    int ch = send(clientSocket, buffer, sizeof(request), 0);
    if(ch < 0){
        printf("Failed to send response to client");
        logerror("Failed to send response to client");
    }

    return;
}

void connectToSS(StorageServer ss, int ssSocket){
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ss.ssPort);
    serv_addr.sin_addr.s_addr = inet_addr(ss.ssIP);    

    int ch = -1;

    //first check if the storage server was previously connected by checking the port number
    for(int i = 0; i < currentServerCount; i++){
        if(storageServersList[i]->ssPort == ss.ssPort){
            ch = i;
            storageServersList[i]->status = 1;
            // if(storageServersList[i]->is_BackedUp) init_redundancy(storageServersList[i]);
            return;
        }
    }
          
    for(int i = 0 ; i < MAX_SERVERS ; i++){
        if(storageServersList[i]->status == -1){
            if(currentServerCount >= 2){
                // pick any two random storage servers as backups. Note that storage servers are not contiguous in the array
                int x = rand() % currentServerCount;
                for(int j = 0; j < currentServerCount && storageServersList[j]->status == 1; j++){
                    if(j == x) storageServersList[i]->backupPort1 = storageServersList[j]->ssPort;
                }
                x = (x+1)%currentServerCount;
                for(int j = 0; j < currentServerCount && storageServersList[j]->status == 1; j++){
                    if(j == x) storageServersList[i]->backupPort2 = storageServersList[j]->ssPort;
                }

            storageServersList[i]->is_BackedUp = 1;
            ch = i;
            printf("Storage server is backed up in %d and %d\n", storageServersList[i]->backupPort1, storageServersList[i]->backupPort2);
            }
            else {
                storageServersList[i]->is_BackedUp = 0;
                storageServersList[i]->backupPort1 = -1;
                storageServersList[i]->backupPort2 = -1;
            }

            strcpy(storageServersList[i]->ssIP, ss.ssIP);
            storageServersList[i]->ssPort = ss.ssPort;
            storageServersList[i]->clientPort = ss.clientPort;
            storageServersList[i]->root = initialize_node();
            storageServersList[i]->numberOfPaths = 0;
            storageServersList[i]->status = 1;
            currentServerCount++;
            //take the list of accessible paths from the storage server
            printf("Waiting for paths from storage server\n");
            char buffer[MAX_STRUCT_LENGTH];
            memset(buffer, 0, sizeof(buffer));

            int size  = recv(ssSocket, buffer, sizeof(buffer), 0);

            if(size <= 0){
                printf("Failed to receive paths from storage server\n");
                logerror("Failed to receive paths from storage server");
            }
            logrecvfrom("Storage Server", ss.ssIP, ss.ssPort, buffer);

            char* token = strtok(buffer, " ");
            pthread_mutex_lock(&storageServersList[i]->mutex);
            int first = 0;
            while(token != NULL){
                if(first == 0){
                    first = 1;
                    memset(storageServersList[i]->backupFolder, 0, sizeof(storageServersList[i]->backupFolder));
                    strcpy(storageServersList[i]->backupFolder, token);
                }
                add_path(storageServersList[i]->root, token, i);
                storageServersList[i]->numberOfPaths++;
                token = strtok(NULL, " ");
            }
            pthread_mutex_unlock(&storageServersList[i]->mutex);
            pthread_t hb;
            pthread_create(&hb, NULL, heartbeat, (void*)storageServersList[i]);
            printf("Received paths from storage server\n");
            break;
        }    
    }
    close(ssSocket);   

    if(currentServerCount == 3){
        //back the initial 2 storage servers also
        int arr[2] = {0};
        int x = 0;
        for(int i = 0; i < MAX_SERVERS; i++){
            if(storageServersList[i]->status == 1 && storageServersList[i]->is_BackedUp == 0 && i != ch){ 
                arr[x++] = i;
            }
        }

        //now backup the initial 2 storage servers into each other and ch
        storageServersList[arr[0]]->is_BackedUp = 1;
        storageServersList[arr[0]]->backupPort1 = storageServersList[arr[1]]->ssPort;
        storageServersList[arr[0]]->backupPort2 = storageServersList[ch]->ssPort;
        printf("Storage server is backed up in %d and %d\n", storageServersList[arr[0]]->backupPort1, storageServersList[arr[0]]->backupPort2);

        storageServersList[arr[1]]->is_BackedUp = 1;
        storageServersList[arr[1]]->backupPort1 = storageServersList[arr[0]]->ssPort;
        storageServersList[arr[1]]->backupPort2 = storageServersList[ch]->ssPort;
        printf("Storage server is backed up in %d and %d\n", storageServersList[arr[1]]->backupPort1, storageServersList[arr[1]]->backupPort2);

        PathList* paths = initialize_path_list();
        collect_paths(paths, storageServersList[arr[0]]->root);

        if(paths == NULL) return;

        PathNode* temp = paths->head;
        while(temp != NULL){
            if(strstr(temp->path, ".wav") != NULL){
                //dont backup audio files
                temp = temp->next;
                continue;   
            }
            if(strstr(temp->path, ".") != NULL){
                printf("Backing up file %s\n", temp->path);
                backup(storageServersList[arr[0]]->clientPort, arr[0], temp->path, COPYFILE);
            }
            else{
                printf("Backing up folder %s\n", temp->path);
                backup(storageServersList[arr[0]]->clientPort, arr[0], temp->path, COPYFOLDER);
            }
            temp = temp->next;
        }
        release_path_list(paths);

        // sleep(1);

        paths = initialize_path_list();
        collect_paths(paths, storageServersList[arr[1]]->root);

        if(paths == NULL) return;

        temp = paths->head;
        while(temp != NULL){
            if(strstr(temp->path, ".wav") != NULL){
                //dont backup audio files
                temp = temp->next;
                continue;   
            }
            if(strstr(temp->path, ".") != NULL){
                printf("Backing up file %s\n", temp->path);
                backup(storageServersList[arr[1]]->clientPort, arr[1], temp->path, COPYFILE);
            }
            else{
                printf("Backing up folder %s\n", temp->path);
                backup(storageServersList[arr[1]]->clientPort, arr[1], temp->path, COPYFOLDER);
            }
            temp = temp->next;
        }
        release_path_list(paths);
    }

    //get the list of paths in trie so that we can backup the file
    if(ch != -1){    
        PathList* paths = initialize_path_list();
        collect_paths(paths, storageServersList[ch]->root);

        if(paths == NULL) return;

        PathNode* temp = paths->head;
        while(temp != NULL){
            if(strstr(temp->path, ".wav") != NULL){
                //dont backup audio files
                temp = temp->next;
                continue;   
            }
            if(strstr(temp->path, ".") != NULL){
                printf("Backing up file %s\n", temp->path);
                backup(storageServersList[ch]->clientPort, ch, temp->path, COPYFILE);
            }
            else{
                printf("Backing up folder %s\n", temp->path);
                backup(storageServersList[ch]->clientPort, ch, temp->path, COPYFOLDER);
            }
            temp = temp->next;
        }
        release_path_list(paths);  
    }
    return;
}

void handleCopyingFolders(processRequestStruct* req){
    char path_source[MAX_PATH_LENGTH] = {0}, path_dest[MAX_PATH_LENGTH] = {0};
    char* token = strtok(req->data, " ");
    strcpy(path_source, token);
    token = strtok(NULL, " ");
    strcpy(path_dest, token);

    int ssid1 = retrievePathIndex(path_source);
    int ssid2 = retrievePathIndex(path_dest);
    if(ssid1 == -1 || ssid2 == -1){
        printf("Invalid paths provided\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Invalid paths provided");
        return;
    }
    else if(ssid1 == -2 || ssid2 == -2){
        printf("Storage server is down\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
        return;
    }

    //check if the folder path provided is a substring of any paths being written to. If yes, return error
    writePathNode* temp = writePathsLL;
    while(temp != NULL){
        if(strstr(temp->path, path_source) != NULL){
            printf("Path is already being written to\n");
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
            return;
        }
        temp = temp->next;
    }

    // insert the path being written to so that no other client can write to it
    insertWritePath(&writePathsLL, req->clientID, path_source, storageServersList[ssid2]->ssPort, path_dest);


    StorageServer* ss_source = storageServersList[ssid1], *ss_dest = storageServersList[ssid2];

    request req1;
    memset(&req1, 0, sizeof(req1));
    sprintf(req1.data, "%s %d %s %s",ss_dest->ssIP, ss_dest->clientPort, path_source, path_dest);
    req1.requestType = COPYFOLDER;

    int fd_source = connectTo(ss_source->ssPort, ss_source->ssIP);

    char data[MAX_STRUCT_LENGTH] = {0};
    memcpy(data, &req1, MAX_STRUCT_LENGTH);
    int c1 = send(fd_source, data, MAX_STRUCT_LENGTH, 0);
    if(c1 < 0) {
        //later
    }
    logsentto("Storage Server", ss_dest->ssIP, ss_dest->ssPort, req1.data);

    printf("Copy request sent.\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, "Copy started");
    return;
}

void handleCopyingFiles(processRequestStruct* req){
    char path_source[MAX_PATH_LENGTH] = {0}, path_dest[MAX_PATH_LENGTH] = {0};
    char* token = strtok(req->data, " ");
    strcpy(path_source, token);
    token = strtok(NULL, " ");
    strcpy(path_dest, token);
    printf("Source: %s ", path_source);
    printf("Dest: %s\n", path_dest);
    int ssid1 = retrievePathIndex(path_source);
    int ssid2 = retrievePathIndex(path_dest);


    if(ssid1 == -1 || ssid2 == -1){
        printf("Invalid paths provided\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Invalid paths provided");
        return;
    }
    else if(ssid1 == -2 || ssid2 == -2){
        printf("Storage server is down\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
        return;
    }

    //check if file is already being written to
    if(tryWritePath(&writePathsLL, path_source) == 0){
        printf("Path is already being written to\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }

    //get the name after the last / from source
    char* temp = strrchr(path_source, '/');
    if(temp == NULL){
        printf("Invalid paths provided\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Invalid paths provided");
        return;
    }
    temp++;

    char nameOfFile[MAX_PATH_LENGTH] = {0};
    strcpy(nameOfFile, path_dest);
    strcat(nameOfFile, "/");
    strcat(nameOfFile, temp);
    
    insertWritePath(&writePathsLL, req->clientID, path_source, storageServersList[ssid2]->ssPort, nameOfFile);

    StorageServer* ss_source = storageServersList[ssid1], *ss_dest = storageServersList[ssid2];

    request req1;
    memset(&req1, 0, sizeof(req1));
    sprintf(req1.data, "%s %d %s %s",ss_dest->ssIP, ss_dest->clientPort, path_source, path_dest);
    req1.requestType = COPYFILE;

    int fd_source = connectTo(ss_source->ssPort, ss_source->ssIP);

    char data[MAX_STRUCT_LENGTH] = {0};
    memcpy(data, &req1, MAX_STRUCT_LENGTH);
    int c1 = send(fd_source, data, MAX_STRUCT_LENGTH, 0);
    if(c1 < 0) {
        //later
    }
    logsentto("Storage Server", ss_dest->ssIP, ss_dest->ssPort, req1.data);

    printf("Copy request sent.\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, "Copy request sent");
}

void handleClientSSRequests(processRequestStruct* req){
    client_request cr;
    memset(&cr, 0, sizeof(client_request));
    //req->data has path
    strcpy(cr.path, req->data);
    printf("Path: %s\n", cr.path);  

    int readingFromDownServer = 0; 
    

    // check if path is already being written to
    if(tryWritePath(&writePathsLL, req->data) == 0){
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }

    //find storage server with the path
    StorageServer* ss;
    int checkPathIfPresent = retrievePathIndex(cr.path);
    if(checkPathIfPresent == -1){
        printf("Path not found\n");
        printf("%s\n", cr.path);
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }   
    else if(checkPathIfPresent == -2){
        if(req->requestType != READ){
            printf("Storage server is down\n");
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
            return;
        }
        else{
            // find the backup storage server
            for(int i = 0; i < currentServerCount; i++){
                pthread_mutex_lock(&storageServersList[i]->mutex);
                if(find_path(storageServersList[i]->root, cr.path) >= 0){
                    if(storageServersList[i]->is_BackedUp == 1){
                        checkPathIfPresent = i;
                        readingFromDownServer = 1;
                        pthread_mutex_unlock(&storageServersList[i]->mutex);
                        break;
                    }
                }
                pthread_mutex_unlock(&storageServersList[i]->mutex);
            }

            if(readingFromDownServer == 0){
                printf("Storage server is down\n");
                sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
                return;
        }
    }
    }

    if(readingFromDownServer == 1){
        // redirect to backup storage server
        // first check if the backups are up
        StorageServer* backup1 = storageServersList[getSSIDFromPort(storageServersList[checkPathIfPresent]->backupPort1)];
        StorageServer* backup2 = storageServersList[getSSIDFromPort(storageServersList[checkPathIfPresent]->backupPort2)];

        if(backup1->status == 0 && backup2->status == 0){
            printf("Backup storage server is down\n");
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Backup storage server is down");
            return;
        }

        //choose the alive backup storage server
        StorageServer* backup;
        if(backup1->status == 1){
            backup = backup1;
        }
        else{
            backup = backup2;
        }

        char data[MAX_STRUCT_LENGTH];
        memset(data, 0, sizeof(data));
        snprintf(data, MAX_STRUCT_LENGTH, "%s %d %s/%s", backup->ssIP, backup->clientPort, backup->backupFolder, cr.path);

        printf("Sending IP and Port of backup server to client\n");
        sendMessageToClient(clientSockets[req->clientID], ACK, data);
        return;
    }
    
    ss = storageServersList[checkPathIfPresent];

    if(req->requestType == WRITEASYNC || req->requestType == WRITESYNC){
        insertWritePath(&writePathsLL, req->clientID, req->data, ss->ssPort, "");
    }
    //send ip and port of ss to client for them to directly connect
    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    if(req->requestType == READ) snprintf(data, MAX_STRUCT_LENGTH, "%s %d %s", ss->ssIP, ss->clientPort, cr.path);
    else snprintf(data, MAX_STRUCT_LENGTH, "%s %d", ss->ssIP, ss->clientPort);

    enqueueLRU(&lruCache, cr.path, checkPathIfPresent);

    printf("Sending IP and Port to client\n");
    sendMessageToClient(clientSockets[req->clientID], ACK, data);
    return;
}


void handleDelete(processRequestStruct* req){
    char data[MAX_PATH_LENGTH];
    memset(data, 0, sizeof(data));
    strcpy(data, req->data);

    // check if path is already being written to
    if(tryWritePath(&writePathsLL, req->data) == 0){
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path is already being written to");
        return;
    }


    //find storage server with the path
    int checkPathIfPresent = retrievePathIndex(data);
    if(checkPathIfPresent == -1){
        printf("Path not found\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
        return;
    }   
    else if(checkPathIfPresent == -2){
        printf("Storage server is down\n");
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
        return;
    }

    StorageServer* ss = storageServersList[checkPathIfPresent];

    insertWritePath(&writePathsLL, req->clientID, req->data, ss->ssPort, "");
    
    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to connect to storage server");

        close(sockfd);
        return;
    }

    request reqq;
    memset(&reqq, 0, sizeof(request));
    reqq.requestType = req->requestType;
    memcpy(reqq.data, req->data, sizeof(client_request));

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &reqq, sizeof(request));

    int ch = send(sockfd, buffer, sizeof(request), 0);
    if(ch < 0){
        printf("Failed to send request to storage server");
        logerror("Failed to send request to storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to send request to storage server");

        close(sockfd);
        return;
    }
    logsentto("Storage Server", ss->ssIP, ss->ssPort, reqq.data);

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);

    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server\n");
        logerror("Failed to receive response from storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to receive response from storage server");

        close(sockfd);
        return;
    }
    request res;
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    logrecvfrom("111Storage Server", ss->ssIP, ss->ssPort, res.data);

    if(res.requestType == ACK){
        printf("File/Folder deleted successfully\n");
        // delete path from the trie
        pthread_mutex_lock(&ss->mutex);
        remove_path(ss->root, data);
        ss->numberOfPaths--;
        pthread_mutex_unlock(&ss->mutex);

        removeFromLRU(&lruCache, data);
        
        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder deleted successfully");

        if(req->requestType == DELETEFILE){
            backup(ss->clientPort, checkPathIfPresent, data, DELETEFILE);
        }
        else{
            backup(ss->clientPort, checkPathIfPresent, data, DELETEFOLDER);
        }
    }
    else{
        printf("Error : %s\n", res.data);
        //sending this to client

        sendMessageToClient(clientSockets[req->clientID], ERROR, res.data);
    }

    close(sockfd);
    return;
}

void listAllPaths(processRequestStruct* req){
    char data[MAX_STRUCT_LENGTH];
    memset(data, 0, sizeof(data));
    for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
        pthread_mutex_lock(&storageServersList[i]->mutex);
        char paths[MAX_STRUCT_LENGTH];
        memset(paths, 0, sizeof(paths));
        store_paths_in_buffer(storageServersList[i]->root, paths);
        strcat(data, paths);
        strcat(data, "\n");
        pthread_mutex_unlock(&storageServersList[i]->mutex);
    }

    // remove all paths with backup as prefix
    char* token = strtok(data, "\n");
    char temp[MAX_STRUCT_LENGTH] = {0};
    while(token != NULL){
        if(strstr(token, "backup") == NULL){
            strcat(temp, token);
            strcat(temp, "\n");
        }
        token = strtok(NULL, "\n");
    }

    sendMessageToClient(clientSockets[req->clientID], ACK, temp);
    return;
}

// handles the Create File/Folder request, takes the request struct as input, searches for the path in the storage servers and sends the request to the appropriate storage server
void handleCreate(processRequestStruct* req){
    char data[MAX_PATH_LENGTH], path[MAX_PATH_LENGTH];
    memset(data, 0, sizeof(data));
    char* token = strtok(req->data, " ");
    strcpy(data, token);
    strcpy(path, data);
    token = strtok(NULL, " ");
    strcat(data, "/");
    strcat(data, token);

    StorageServer* ss;
    int checkPathIfPresent;

    if(strcmp(path, ".") == 0){
        // choose the storage server with least number of paths
        int min = 100000000;
        int index = -1;

        for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
            pthread_mutex_lock(&storageServersList[i]->mutex);
            if(storageServersList[i]->numberOfPaths < min){
                min = storageServersList[i]->numberOfPaths;
                index = i;
            }
            pthread_mutex_unlock(&storageServersList[i]->mutex);
        }

        if(index == -1){
            printf("No storage server available\n");
            //sending this to client
            sendMessageToClient(clientSockets[req->clientID], ERROR, "No storage server available");
            return;
        }
        ss = storageServersList[index];
        checkPathIfPresent = index;
    }
    else {
        checkPathIfPresent = retrievePathIndex(path);
        if(checkPathIfPresent == -1){
            printf("Path not found\n");
            //sending this to client
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Path not found");
            return;
        }
        else if(checkPathIfPresent == -2){
            printf("Storage server is down\n");
            //sending this to client
            sendMessageToClient(clientSockets[req->clientID], ERROR, "Storage server is down");
            return;
        }
        ss = storageServersList[checkPathIfPresent];
    }

    // check if the file/folder already exists
    if(find_path(ss->root, data) >= 0){
        printf("File/Folder already exists\n");
        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "File/Folder already exists");
        return;
    }

    int sockfd = connectTo(ss->ssPort, ss->ssIP);

    if(sockfd < 0){
        printf("Failed to connect to storage server\n");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to connect to storage server");

        close(sockfd);
        return;
    }


    request reqq;
    memset(&reqq, 0, sizeof(request));
    reqq.requestType = req->requestType;
    memcpy(reqq.data, data, sizeof(client_request));

    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &reqq, sizeof(request));

    int ch = send(sockfd, buffer, sizeof(request), 0);    
    if(ch < 0){
        printf("Failed to send request to storage server");
        logerror("Failed to send request to storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to send request to storage server");

        close(sockfd);
        return;
    }
    logsentto("Storage Server", ss->ssIP, ss->ssPort, reqq.data);

    char response[sizeof(request)];

    int bytes_received = recv(sockfd, response, sizeof(response), 0);
    if (bytes_received <= 0) {
        printf("Failed to receive response from storage server");
        logerror("Failed to receive response from storage server");
        ss->status = 0;

        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, "Failed to receive response from storage server");

        close(sockfd);
        return;
    }

    request res; 
    memset(&res, 0, sizeof(request));
    memcpy(&res, response, sizeof(request));

    logrecvfrom("Storage Server", ss->ssIP, ss->ssPort, res.data);
    

    if(res.requestType == ACK && (req->requestType == CREATEFILE || req->requestType == CREATEFOLDER)){
        printf("File/Folder created successfully\n");
        // add path to the trie
        pthread_mutex_lock(&ss->mutex);
        add_path(ss->root, data, checkPathIfPresent);
        ss->numberOfPaths++;
        pthread_mutex_unlock(&ss->mutex);
        

        enqueueLRU(&lruCache, path, checkPathIfPresent);


        sendMessageToClient(clientSockets[req->clientID], ACK, "File/Folder created successfully");

        if(req->requestType == CREATEFILE){
            backup(ss->clientPort, checkPathIfPresent, data, COPYFILE);
        }
        else{
            backup(ss->clientPort, checkPathIfPresent, data, COPYFOLDER);
        }
    }
    else{
        printf("Error : %s\n", res.data);
        //sending this to client
        sendMessageToClient(clientSockets[req->clientID], ERROR, res.data);
    }

    close(sockfd);
    return;
}

// Thread function to process requests
void* processRequests(void* args){
    processRequestStruct* req = (processRequestStruct*)args;
    printf("Processing request\n");

    sendMessageToClient(clientSockets[req->clientID], ACK, "Request received");

    if(req->requestType == INITSS){
        StorageServer ss;
        memset(&ss, 0, sizeof(StorageServer));
        memcpy(&ss, req->data, sizeof(StorageServer));
        printf("Connecting to storage server %s:%d\n", ss.ssIP, ss.ssPort);
        connectToSS(ss, clientSockets[req->clientID]);
        removeFromClientSocket(req->clientID);
        free(req);
    }
    else if(req->requestType == CREATEFOLDER || req->requestType == CREATEFILE){
        // create file
        handleCreate(req);
        removeFromClientSocket(req->clientID);
        free(req);
    }
    else if(req->requestType == DELETEFOLDER || req->requestType == DELETEFILE){
        // delete file
        handleDelete(req);
        removeFromClientSocket(req->clientID);
        free(req);        
    }
    else if(req->requestType == WRITESYNC ||  req->requestType == READ || req->requestType == INFO || req->requestType == STREAM){
        handleClientSSRequests(req); 
        removeFromClientSocket(req->clientID);
        free(req);
    }
    else if(req->requestType == WRITEASYNC ){
        handleClientSSRequests(req);  
        free(req);
    }
    else if(req->requestType == LIST){
        listAllPaths(req);
        removeFromClientSocket(req->clientID);
        free(req);
    }
    else if(req->requestType == ACK){
        // unlock the path
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);
        removeWritePath(&writePathsLL, data);
        removeFromClientSocket(req->clientID);
        free(req);
    }
    else if(req->requestType == WRITE_ACK){
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);

        // backup(req->data, COPYFILE);
        writePathNode* temp = findWritePath(&writePathsLL, data);
        if(temp == NULL){
            printf("Path not found\n");
            return NULL;
        }
        sendMessageToClient(clientSockets[temp->clientID], ACK, "Data was written succesfully.");
        close(clientSockets[temp->clientID]);        
        clientSockets[temp->clientID] = -1;

        char tempData[MAX_PATH_LENGTH];
        memset(tempData, 0, MAX_PATH_LENGTH);
        removeWritePath(&writePathsLL, data);
        backup(temp->portNumber, getSSIDFromPort(temp->portNumber), data, COPYFILE);

        removeFromClientSocket(req->clientID);

        free(req);
    }
    else if(req->requestType == COPYFOLDER){
        handleCopyingFolders(req);
    }
    else if(req->requestType == COPYFILE){
        handleCopyingFiles(req);
    }
    else if(req->requestType == REGISTER_PATH){
        // insert the path in the trie
        char data[MAX_PATH_LENGTH];
        memset(data, 0, MAX_PATH_LENGTH);
        strcpy(data, req->data);
        
        char* token = strtok(data, " ");

        int port = atoi(token);
        char* path = strtok(NULL, " ");
        printf("Registering path %s with port %d\n", path, port);

        //find the ss with the port
        for(int i = 0; i < currentServerCount && storageServersList[i]->status == 1; i++){
            if(storageServersList[i]->ssPort == port){
                pthread_mutex_lock(&storageServersList[i]->mutex);
                add_path(storageServersList[i]->root, path, i);
                storageServersList[i]->numberOfPaths++;
                pthread_mutex_unlock(&storageServersList[i]->mutex);
                break;
            }
        }    

        removeFromClientSocket(req->clientID);

        free(req);
    }
    else if(req->requestType == REGISTER_PATH_STOP_FILE){
        printf("Path %s is no longer being written to\n", req->data);
        writePathNode* temp1 = findWritePath(&writePathsLL, req->data);
        if(temp1 == NULL) {
            return NULL;
            close(clientSockets[req->clientID]);
            clientSockets[req->clientID] = -1;
        }

        int id = temp1->clientID;
        sendMessageToClient(clientSockets[id], ACK, "Process Completed.");
        removeFromClientSocket(id);
        removeFromClientSocket(req->clientID);

        writePathNode* temp = findWritePath(&writePathsLL, req->data);
        backup(temp->portNumber, getSSIDFromPort(temp->portNumber), temp->dest_path, COPYFILE);
        free(req);
    }
    else if(req->requestType == REGISTER_PATH_STOP_FOLDER){
        printf("Path %s is no longer being written to\n", req->data);
        writePathNode* temp1 = findWritePath(&writePathsLL, req->data);
        if(temp1 == NULL) {
            return NULL;
            close(clientSockets[req->clientID]);
            clientSockets[req->clientID] = -1;
        }

        int id = temp1->clientID;

        sendMessageToClient(clientSockets[id], ACK, "Process Completed.");
        removeFromClientSocket(id);
        removeFromClientSocket(req->clientID);

        writePathNode* temp = findWritePath(&writePathsLL, req->data);
        backup(temp->portNumber, getSSIDFromPort(temp->portNumber), temp->dest_path, COPYFOLDER);        
    }
    return NULL;
}

void* heartbeat(void* arg)
{
    StorageServer* ss = (StorageServer*)arg;
    while(1){
        sleep(5);
        int sockfd = connectTo(ss->ssPort, ss->ssIP);
        if(sockfd < 0){
            pthread_mutex_lock(&ss->mutex);
            printf("Storage server %s:%d is down\n", ss->ssIP, ss->ssPort);
            ss->status = 0;
            removefromLRUBySSID(&lruCache, getSSIDFromPort(ss->ssPort));

            
            pthread_mutex_unlock(&ss->mutex);
            close(sockfd);
            return NULL;
        }
        close(sockfd);
    }
}
