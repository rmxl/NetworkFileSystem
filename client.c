#include "./commonheaders.h"
#include "./namingserver.h"
#include "./client.h"

// #define TIMEOUT 1000

#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN "\033[1;36m"
#define RESET "\033[0m"

void *listenForBackgroundAck(void *arg)
{
    AckThreadArgs *args = (AckThreadArgs *)arg;
    int sockfd = args->sockfd;
    int *ack_received = args->ack_received;

    struct timeval timeout2;
    timeout2.tv_sec = 0;  
    timeout2.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout2, sizeof(timeout2)) < 0) {
        perror(RED "Error removing socket timeout2\n" RESET);
        close(sockfd);
        return NULL;
    }

    request ack_packet;
    memset(ack_packet.data, 0, sizeof(ack_packet.data));
    ssize_t bytes_received = recv(sockfd, &ack_packet, sizeof(ack_packet), 0);
    if (bytes_received <= 0)
    {
        if (bytes_received < 0)
            perror(RED "Error: recv() failed\n" RESET);
        *ack_received = 0;
    }
    else
    {
        if (ack_packet.requestType == ACK)
        {
            // printf(GREEN "\nReceived acknowledgment from naming server: %s\n" RESET, ack_packet.data);
            printf(GREEN "%s\n" RESET, ack_packet.data);
            fflush(stdout);
            *ack_received = 1;
            // printf("Operation acknowledged by naming server\n");
        }
        else
        {
            // printf("\nReceived unexpected response type: %d\n", ack_packet.requestType);
            // printf("Received response data: %s\n", ack_packet.data);
            printf(RED "%s" RESET, ack_packet.data);
            printf("\n");
            fflush(stdout);
            *ack_received = 0;
        }
    }
    // printf("\033[0G\n"); 
    // fflush(stdout);
    close(sockfd);
    return NULL;
}

char* normalizePath(char* path) {
    if (strcmp(path, ".") == 0) return ".";
    while (*path == '.' || *path == '/') {
        path++;
    }
    char* normalizedPath = (char*)malloc(strlen(path) + 3); 
    if (!normalizedPath) return NULL; 
    strcpy(normalizedPath, path);
    return normalizedPath;
}

requestType getRequestType(const char* operation) {
    if (strcmp(operation, "READ") == 0) return READ;
    if (strcmp(operation, "WRITESYNC") == 0) return WRITESYNC;
    if (strcmp(operation, "WRITEASYNC") == 0) return WRITEASYNC;
    if (strcmp(operation, "CREATEFILE") == 0) return CREATEFILE; 
    if (strcmp(operation, "CREATEFOLDER") == 0) return CREATEFOLDER;
    if (strcmp(operation, "DELETEFILE") == 0) return DELETEFILE; 
    if (strcmp(operation, "DELETEFOLDER") == 0) return DELETEFOLDER;
    if (strcmp(operation, "COPYFILE") == 0) return COPYFILE;
    if (strcmp(operation, "COPYFOLDER") == 0) return COPYFOLDER;
    if (strcmp(operation, "LIST") == 0) return LIST;
    if (strcmp(operation, "INFO") == 0) return INFO;
    if (strcmp(operation, "STREAM") == 0) return STREAM;
    // if (strcmp(operation, "EXIT") == 0) return INITSS;
    if (strcmp(operation, "ACK") == 0) return ACK;
    if (strcmp(operation, "ERROR") == 0) return ERROR;
    if (strcmp(operation, "STOP") == 0) return STOP; 
    return INITSS;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf(CYAN "Usage: %s <naming server ip>\n" RESET, argv[0]);
        fflush(stdout);
        exit(1);
    }
    char* nsip = argv[1];
    // printf("Naming server IP: %s\n", nsip);
    char input[MAX_NAME_LENGTH];
    char operation[MAX_IPOP_LENGTH], arg1[MAX_PATH_LENGTH], arg2[MAX_PATH_LENGTH];
    printf(CYAN "...............Client starting.............\n" RESET);
    fflush(stdout);

    while (1) {
        printf(CYAN "Enter Command: " RESET); ;
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) {
            fprintf(stderr, "Error reading input.\n");
            continue;
        }
        input[strcspn(input, "\n")] = '\0';
        // printf("Input: %s\n", input);
        arg1[0] = '\0';
        arg2[0] = '\0';
        char* token = strtok(input, " ");
        if (token == NULL) {
            printf(RED "Error: No operation provided.\n" RESET);
            fflush(stdout);
            continue;
        }
        if (strcmp(token, "EXIT") == 0) {
            printf(CYAN "Exiting client...\n" RESET);
            fflush(stdout);
            break;
        }
        strncpy(operation, token, MAX_IPOP_LENGTH);
        operation[MAX_IPOP_LENGTH - 1] = '\0';
        // printf("Operation: %s\n", operation);
        token = strtok(NULL, " ");
        if (token != NULL) {
            strncpy(arg1, token, MAX_PATH_LENGTH);
            arg1[MAX_PATH_LENGTH - 1] = '\0';
            strcpy(arg1, normalizePath(arg1));
        }
        request req;
        req.requestType = getRequestType(operation);
        if (!(strcmp(operation, "READ") == 0 || strcmp(operation, "WRITESYNC") == 0 || strcmp(operation, "WRITEASYNC") == 0 || strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "DELETEFILE") == 0 || strcmp(operation, "DELETEFOLDER") == 0 || strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0 || strcmp(operation, "LIST") == 0 || strcmp(operation, "INFO") == 0 || strcmp(operation, "STREAM") == 0 || strcmp(operation, "EXIT") == 0)) {
            printf(RED "Error: Unrecognized operation.\n" RESET);
            fflush(stdout);
            continue;
        }
        else if (strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0 || strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "CREATEFILE") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                strncpy(arg2, token, MAX_PATH_LENGTH);
                arg2[MAX_PATH_LENGTH - 1] = '\0';
            }
        }
        if (!(req.requestType == LIST) && !arg1[0]) {
            printf(RED "Error: Invalid format, missing primary argument.\n" RESET);
            fflush(stdout);
            continue;
        }
        if ((strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0) && !arg2[0]) {
            printf(RED "Error: Missing second argument for %s operation.\n" RESET, operation);
            fflush(stdout);
            continue;
        }
        if (arg1[0] && arg2[0] && req.requestType != WRITESYNC && req.requestType != LIST) snprintf(req.data, MAX_STRUCT_LENGTH, "%s %s", arg1, arg2);
        else if (arg1[0] && req.requestType != LIST) snprintf(req.data, MAX_STRUCT_LENGTH, "%s", arg1);
        else req.data[0] = '\0';
        // printf("arg1: %s, arg2: %s\n", arg1, arg2);
        // printf("1RequestType: %d, Data: %s\n", req.requestType, req.data);


        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror(RED "Error creating socket\n" RESET);
            continue;
        }
        struct sockaddr_in ns_addr;
        ns_addr.sin_family = AF_INET;
        ns_addr.sin_port = htons(NAME_SERVER_PORT);
        ns_addr.sin_addr.s_addr = inet_addr(nsip);
        if (connect(sockfd, (struct sockaddr*)&ns_addr, sizeof(ns_addr)) < 0) {
            perror(RED "Error connecting to naming server\n" RESET);
            close(sockfd);
            continue;
        }

        if (send(sockfd, &req, sizeof(req), 0) < 0) {
            perror(RED "Error sending request to naming server\n" RESET);
            close(sockfd);
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 10; 
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror(RED "Error setting socket options\n" RESET);
            close(sockfd);
            continue;
        }

        request ns_ack;
        memset(ns_ack.data, 0, sizeof(ns_ack.data));
        if (recv(sockfd, &ns_ack, sizeof(ns_ack), 0) < 0) {
            perror(RED "Error receiving response from naming server\n" RESET);
            close(sockfd);
            continue;
        }
        
        if (ns_ack.requestType != ACK) {
            printf(RED "Error: Unexpected response from naming server. RequestType: %d\n" RESET, ns_ack.requestType);
            fflush(stdout);
            close(sockfd);
            continue;
        }
        // if (req.requestType == LIST) printf("Received ACK from naming server. Data: %s\n", ns_ack.data);
        // if (!(req.requestType == WRITEASYNC || req.requestType == COPYFILE || req.requestType == COPYFOLDER)) printf(CYAN "%s\n" RESET, ns_ack.data);

        if (req.requestType == COPYFILE || req.requestType == COPYFOLDER) goto COPY; 

        request ns_response;
        memset(ns_response.data, 0, sizeof(ns_response.data));
        if (recv(sockfd, &ns_response, sizeof(ns_response), 0) < 0) {
            perror(RED "Error receiving response from naming server\n" RESET);
            close(sockfd);
            // exit(1);
            continue;
        }
        if (req.requestType != WRITEASYNC && req.requestType != COPYFILE && req.requestType != COPYFOLDER) close(sockfd);
        // printf("Naming Server Response - RequestType: %d, \nData: %s\n", ns_response.requestType, ns_response.data);
        // printf(CYAN "%s\n" RESET, ns_response.data);
        if (ns_response.requestType == ERROR || req.requestType == LIST) {
            printf(CYAN "%s\n" RESET, ns_response.data);
            memset(ns_response.data, 0, sizeof(ns_response.data));
            fflush(stdout);
            continue;
        }
        // printf("2RequestType: %d, Data: %s\n", req.requestType, req.data);
        // printf(CYAN "%s\n" RESET, req.data);
        if (req.requestType == LIST) {
            // printf("printed list items above");
            continue;
        }    
        
        char storage_server_ip[MAX_IPOP_LENGTH], storage_server_port[MAX_IPOP_LENGTH], readpath[MAX_PATH_LENGTH];
        memset(storage_server_ip, 0, sizeof(storage_server_ip));
        memset(storage_server_port, 0, sizeof(storage_server_port));
        memset(readpath, 0, sizeof(readpath));
        if (req.requestType == READ) {
            if (sscanf(ns_response.data, "%s %s %s", storage_server_ip, storage_server_port, readpath) != 3) {
                printf(RED "Error: Invalid response from naming server.\n" RESET);
                fflush(stdout);
                continue;
            }
        }
        else {
            if (sscanf(ns_response.data, "%s %s", storage_server_ip, storage_server_port) != 2) {
                printf(RED "Error: Invalid response from naming server.\n" RESET);
                fflush(stdout);
                continue;
            }
        }

        if (req.requestType == READ) {
            memset(req.data, 0, sizeof(req.data));
            strcpy(req.data, readpath);
        }
        // printf("%s\n", readpath);

        // printf("Storage Server IP: %s, Port: %s\n", storage_server_ip, storage_server_port);
        // fflush(stdout);
        COPY:

        // printf("operation: %s\n", operation);
        if (strcmp(operation, "CREATEFILE") == 0 || strcmp(operation, "CREATEFOLDER") == 0 || strcmp(operation, "DELETEFILE") == 0 || strcmp(operation, "DELETEFOLDER") == 0 || strcmp(operation, "COPY") == 0) {
            continue;
        }
        else if (strcmp(operation, "COPYFILE") == 0 || strcmp(operation, "COPYFOLDER") == 0) {
            int ack_received = 0;
            pthread_t ack_thread;
            AckThreadArgs thread_args = {.sockfd = sockfd, .ack_received = &ack_received};
            pthread_create(&ack_thread, NULL, listenForBackgroundAck, (void *)&thread_args);
            // printf("COPY FILE/FOLDER operation sent - acknowledgment will be received in background\n");
            continue;
        }

        // printf("Connecting to storage server...\n");    

        int ss_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (ss_sockfd < 0) {
            perror(RED "Error creating socket\n" RESET);
            continue;
            // exit(1);
        }
        struct sockaddr_in ss_addr;
        ss_addr.sin_family = AF_INET;
        ss_addr.sin_port = htons(atoi(storage_server_port));
        ss_addr.sin_addr.s_addr = inet_addr(storage_server_ip);
        if (connect(ss_sockfd, (struct sockaddr*)&ss_addr, sizeof(ss_addr)) < 0) {
            perror(RED "Error connecting to storage server\n" RESET);
            close(ss_sockfd);
            // exit(1);
            continue;
        }
        if (setsockopt(ss_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror(RED "Error setting socket options\n" RESET);
            close(sockfd);
            continue;
        }
        if (send(ss_sockfd, &req, sizeof(req), 0) < 0)
        {
            perror(RED "Error sending request to storage server\n" RESET);
            close(ss_sockfd);
            // exit(1);
            continue;
        }
        // printf("request data: %s", req.data);
        int send_write_flag = 1;
        if (req.requestType == WRITEASYNC || req.requestType == WRITESYNC)
        {
            printf(CYAN "Enter content to write (type 2 consecutive enters to stop):\n" RESET);
            char buffer[10];
            memset(buffer, 0, sizeof(buffer));
            int consecutive_newlines = 0;
            ssize_t bytes_sent;
            while (1)
            {
                if (!fgets(buffer, sizeof(buffer), stdin))
                {
                    perror(RED "Error reading input\n" RESET);
                    break;
                }
                if (buffer[0] == '\n')
                {
                    consecutive_newlines++;
                    if (consecutive_newlines == 1)
                        break; 
                }
                else
                    consecutive_newlines = 0; 
                
                request ss_send;
                ss_send.requestType = req.requestType;
                memset(ss_send.data, 0, sizeof(ss_send.data));
                strncpy(ss_send.data, buffer, sizeof(ss_send.data) - 1);
                bytes_sent = send(ss_sockfd, &ss_send, sizeof(ss_send), 0);
                if (bytes_sent < 0)
                {
                    send_write_flag = 0;
                    perror(RED "Error sending data to storage server\n" RESET);
                    close(ss_sockfd);
                    // exit(EXIT_FAILURE);
                    break;
                }
            }
            if (send_write_flag == 0) continue;
            // printf("Sending stop\n");
            request ss_send;
            ss_send.requestType = STOP;
            memset(ss_send.data, 0, sizeof(ss_send.data));
            strncpy(ss_send.data, "STOP", sizeof(ss_send.data) - 1);
            bytes_sent = send(ss_sockfd, &ss_send, sizeof(ss_send), 0);
            if (bytes_sent < 0)
            {
                perror(RED "Error sending STOP to storage server\n" RESET);
                close(ss_sockfd);
                // exit(EXIT_FAILURE);
                continue;
            }
            // printf("Write operation complete.\n");
        }
        // printf("Request sent to storage server.\n");


        if (req.requestType == STREAM) {
            char buffer[MAX_STRUCT_LENGTH];
            FILE *audio_player = popen("ffplay -autoexit -nodisp -", "w");
            if (audio_player == NULL) {
                perror(RED "Error opening audio player\n" RESET);
                close(sockfd);
                // exit(EXIT_FAILURE);
                continue;
            }
            ssize_t bytes_received;
            while ((bytes_received = recv(ss_sockfd, buffer, MAX_STRUCT_LENGTH, 0)) > 0) {
                fwrite(buffer, 1, bytes_received, audio_player);
                fflush(audio_player);
            }
            if (bytes_received < 0) {
                perror(RED "Error receiving audio data\n" RESET);
                continue;
            }
            // else printf("Audio stream ended\n");
            pclose(audio_player);
            close(ss_sockfd);
        }
        else if (req.requestType == READ) {
            request ss_response;
            memset(ss_response.data, 0, sizeof(ss_response.data));
            ssize_t bytes_received;
            // printf("Starting READ operation...\n");
            while ((bytes_received = recv(ss_sockfd, &ss_response, sizeof(ss_response), 0)) > 0)
            {
                if (req.requestType != ACK && strcmp(ss_response.data, "File data sent successfully") != 0) printf("%s", ss_response.data);
            }
            printf("\n");
            fflush(stdout);
            if (bytes_received < 0)
                perror(RED "Error receiving data for READ request\n" RESET);
            // else
            //     printf("READ operation completed, no more data.\n");
            close(ss_sockfd);
        }
        else {
            request ss_response;
            memset(ss_response.data, 0, sizeof(ss_response.data));
            if (recv(ss_sockfd, &ss_response, sizeof(ss_response), 0) < 0) {
                perror(RED "Error receiving response from storage server\n" RESET);
                close(ss_sockfd);
                continue;
            }          
            // printf("Storage Server Response - RequestType: %d, Data: %s\n", ss_response.requestType, ss_response.data);
            printf(CYAN "%s\n" RESET, ss_response.data);
            fflush(stdout);
            if (req.requestType == WRITEASYNC)
            {
                int ack_received = 0;
                pthread_t ack_thread;
                AckThreadArgs thread_args = {.sockfd = sockfd, .ack_received = &ack_received};
                pthread_create(&ack_thread, NULL, listenForBackgroundAck, (void *)&thread_args);
                // printf("Write operation sent - acknowledgment will be received in background\n");
                continue;
            }
            close(ss_sockfd);
        }        
    }
    return 0;
}
