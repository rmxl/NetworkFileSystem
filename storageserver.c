#include "./storageserver.h"
#include "./commonheaders.h"
#include "namingserver.h"
#include <pthread.h>
#include "ss_functions.h"
#include <errno.h>
#define QUEUE_SIZE 1024
// #define PORT 8083
#define CLIENT_PORT PORT+1
int ns_send_socket;
int queue[QUEUE_SIZE];
int front = 0;
int rear = -1;
int itemcount = 0;
int peek() {
    return queue[front];
}
int isEmpty() {
    return itemcount == 0;
}
int isFull() {
    return itemcount == 1024;
}
int size() {
    return itemcount;
}
void insert(int data) {
    if (!isFull()) {
        if (rear == 1024 - 1) {
            rear = -1;
        }
        queue[++rear] = data;
        itemcount++;
    }
}
int removeData() {
    int data = queue[front++];
    if (front == 1024) {
        front = 0;
    }
    itemcount--;
    return data;
}
// int sockfd;
int connect_to_ns(char *ns_ip, int ns_port,char* argv) {
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    // Get server information
    server = gethostbyname(ns_ip);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(sockfd);
        return -1;
    }

    // Set up server address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(ns_port);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        int errsv = errno;
        printf("errno: %d\n", errsv);
        close(sockfd);
        return -1;
    }
    ns_send_socket = sockfd;
    StorageServer ss;
    ss.root = NULL;
    pthread_mutex_init(&ss.mutex, NULL);
    // ss.ssIP = (char*)malloc(strlen(argv) + 1);
    strcpy(ss.ssIP, argv );
    ss.ssPort = PORT;
    ss.clientPort = CLIENT_PORT;
    printf("client port: %d\n", ss.clientPort);
    char buffer[sizeof(StorageServer)];
    memset(buffer, 0, sizeof(buffer));
    memcpy(buffer, &ss, sizeof(StorageServer));
    // printf all the information
    printf("IP: %s\n", ss.ssIP);
    printf("Port: %d\n", ss.ssPort);
    printf("Root: %p\n", ss.root);
    // printf("Write Lock: %p\n", ss.writeLock);
    request req;
    memset(&req, 0, sizeof(request));
    req.requestType = INITSS;
    memcpy(req.data, buffer, sizeof(buffer));
    char packet[sizeof(request)];
    memset(packet, 0, sizeof(packet));
    memcpy(packet, &req, sizeof(request));

    printf("packet: %s\n", packet);
    int ch=send(sockfd, packet, sizeof(packet), 0);
    if(ch<0){
        perror("Failed to send storage server information");
        close(sockfd);
        return 1;
    }
    // send the accessible paths to the naming server
    FILE *fp = fopen("./accessible_paths.txt", "r");
    if (fp == NULL) {
        perror("Error opening file");
        close(sockfd);
        return -1;
    }
    char buffer1[1024];
    memset(buffer1, 0, sizeof(buffer1));
    while (fgets(buffer1, sizeof(buffer1), fp) != NULL) {
        buffer1[strlen(buffer1) - 1] = ' ';
        char * buffer2 = buffer1;
        printf("buffer1: %s\n", buffer1);
        int sz = strlen(buffer1);
        while(sz > 0){
            int sent = send(sockfd, buffer2, sz, 0);
            if(sent < 0) {
                printf("Error sending data\n");
                fclose(fp);
                close(sockfd);
                return -1;
            }
            sz -= sent;
            buffer2 += sent;
        }
    }
    // Send IP, port, and identifier information to the Name Server
   
    // char buffer[1024];
    // recv(sockfd, buffer, sizeof(buffer), 0);

    // // Open the accessible paths file
    // FILE *fp = fopen("./accessiblepaths.txt", "r");
    // if (fp == NULL) {
    //     perror("Error opening file");
    //     close(sockfd);
    //     return -1;
    // }

    // // Read and send the contents of accessiblepaths.txt to the server
    // memset(buffer, 0, sizeof(buffer));
    // while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    //     if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
    //         perror("ERROR sending data");
    //         fclose(fp);
    //         close(sockfd);
    //         return -1;
    //     }
    // }
    // fclose(fp);
     if(close(sockfd)<0){
        perror("Failed to close socket");
        return -1;
     }
    return sockfd;
}

int main(int argc, char *argv[]) {
    // Verify command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NameServer_IP> <name of ss>\n", argv[0]);
        return 1;
    } 
    //change the current working directory to the directory where the storage server is running
    if(chdir(argv[2])<0){
        perror("Failed to change directory");
        return 1;
    }
    printf("Current working directory: %s\n", getcwd(NULL, 0));
    // return 1;
    // Connect to the Name Server
    int ns_socket = connect_to_ns(argv[1],NAME_SERVER_PORT ,argv[1]);
    if (ns_socket == -1) {
        printf("Connection to Name Server failed\n");
        return 1;
    }
    printf("Connection to Name Server successful\n");
    
    // Close the socket after the communication is done
    close(ns_socket);
    pthread_t ns_listener;
    pthread_create(&ns_listener, NULL, NS_listener, argv[1]);
    // sleep(0.1) ;
    pthread_t client_listener;
    pthread_create(&client_listener, NULL, Client_listner, argv[1]);
    // pthread_create(&Iamalive, NULL, Iamalive_thread, argv[1]);
    pthread_join(ns_listener, NULL);
    pthread_join(client_listener, NULL);

    return 0;
}
