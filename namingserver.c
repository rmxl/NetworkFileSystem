#include "./namingserver.h"
#include "./requests.h"
#include "./commonheaders.h"

int serverPorts = 8082;
StorageServer* storageServersList[MAX_SERVERS];
int currentServerCount = 0;
int sockfdMaster;


int clientSockets[MAX_CLIENTS];

LRUList* lruCache;

writePathNode* writePathsLL;
pthread_mutex_t writePathsLLMutex;

pthread_mutex_t clientSocketsLock = PTHREAD_MUTEX_INITIALIZER;

int findFreeClientSocketIndex() {
    pthread_mutex_lock(&clientSocketsLock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientSockets[i] == -1) {
            pthread_mutex_unlock(&clientSocketsLock);
            return i;
        }
    }
    pthread_mutex_unlock(&clientSocketsLock);
    return -1;
}

void setupConnectionToClient() {
    printf("Starting to set up connections...\n");

// Set up the socket once, before the loop
struct sockaddr_in servaddr;
sockfdMaster = socket(AF_INET, SOCK_STREAM, 0);
if (sockfdMaster == -1) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
}
printf("Socket successfully created...\n");

bzero(&servaddr, sizeof(servaddr)); 
servaddr.sin_family = AF_INET;
servaddr.sin_addr.s_addr = INADDR_ANY;
servaddr.sin_port = htons(SERVER_PORT);
setsockopt(sockfdMaster, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

if (bind(sockfdMaster, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Bind failed");
    close(sockfdMaster);
    exit(EXIT_FAILURE);
}
printf("Bind to port %d successful...\n", SERVER_PORT);

if (listen(sockfdMaster, MAX_CLIENTS) < 0) {
    perror("Listen failed");
    close(sockfdMaster);
    exit(EXIT_FAILURE);
}   

while (1) {
    int clientSocketID = findFreeClientSocketIndex();
    if (clientSocketID == -1) {
        printf("No free client sockets");
        continue;  // Continue instead of exiting
    }

    struct sockaddr_in clientaddr;
    int len = sizeof(clientaddr);
    clientSockets[clientSocketID] = accept(sockfdMaster, (struct sockaddr *)&clientaddr, (socklen_t*)&len);

    if (clientSockets[clientSocketID] < 0) {
        perror("Client accept failed");
        continue;
    }
    printf("Client connected on socket %d\n", clientSockets[clientSocketID]);

    request* req = (request*)malloc(sizeof(request));
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received = recv(clientSockets[clientSocketID], buffer, sizeof(request), 0);
    if (bytes_received <= 0) {
        perror("Failed to receive request");

        close(clientSockets[clientSocketID]);
        clientSockets[clientSocketID] = -1;
        free(req);
        continue;
    }
    memcpy(req, buffer, sizeof(request));
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientaddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    logrecvfrom("Client", clientIP, ntohs(clientaddr.sin_port), req->data);
    printf("Received request from client on socket %d\n", clientSockets[clientSocketID]);
    processRequestStruct* prs = (processRequestStruct*)malloc(sizeof(processRequestStruct));
    prs->requestType = req->requestType;
    prs->clientID = clientSocketID;
    memcpy(prs->data, req->data, sizeof(req->data));
    free(req);
    pthread_t requestHandlerThread;
    if (pthread_create(&requestHandlerThread, NULL, processRequests, (void*)prs) != 0) {
        perror("Failed to create request handler thread");
        free(prs);
        close(clientSockets[clientSocketID]);
        clientSockets[clientSocketID] = -1;
    } 
}
    close(sockfdMaster);
    return;
}

int main(int argc, char *argv[]) {
    for(int i = 0; i < MAX_CLIENTS; i++)
        clientSockets[i] = -1;
    printf("Starting naming server...\n");

    FILE* logFile = fopen("logs.txt", "w");
    if(logFile == NULL){
        perror("Error opening log file");
        // exit(EXIT_FAILURE);
    }
    fprintf(logFile, "Logs file created successfully...\n");
    fclose(logFile);

    for(int i = 0 ; i < MAX_SERVERS ; i++){
        storageServersList[i] = (StorageServer*)malloc(sizeof(StorageServer));
        storageServersList[i]->status = -1;
        storageServersList[i]->numberOfPaths = -1;
        storageServersList[i]->clientPort = 0;
        storageServersList[i]->root = NULL;
        pthread_mutex_init(&storageServersList[i]->mutex, NULL);
    }

    lruCache = createLRUList(MAX_LRU_SIZE);
    writePathsLL = NULL;
    pthread_mutex_init(&writePathsLLMutex, NULL);

    setupConnectionToClient();
    return 0;
}
