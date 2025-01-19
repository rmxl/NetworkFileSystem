#include "./storageserver.h"
// #include "./requests.h"
#include "./commonheaders.h"
#include "namingserver.h"
#include "./ss_functions.h"
#include <errno.h>
#include "./lru.h"

#define CLIENT_PORT PORT+1
void* send_write_ack_to_ns(char* filename)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }

    struct hostent *server;
    server = gethostbyname(NS_IP);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(NS_PORT);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        return NULL;
    }
    request pack;
    pack.requestType = WRITE_ACK;
    strcpy(pack.data, filename);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(sockfd, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sockfd);
        return NULL;
    }
    return NULL;

}
void* send_err_to_ns(int Socket,char *msg)
{
    request pack;
    pack.requestType = ERROR;
    strcpy(pack.data, msg);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(Socket, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(Socket);
        return NULL;
    }
    return NULL;
}
int compare(const FTSENT** one, const FTSENT** two)
{
    return (strcmp((*one)->fts_name, (*two)->fts_name));
}
int isFile(const char* path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    if(S_ISREG(path_stat.st_mode)) return 1;
    return 0;
}
void* fts_search( char* arg,int clientSocket,char* dest)

{
    FTS* file_system=NULL;
    FTSENT *node=NULL;
    request pack1;
    pack1.requestType=PASTEFOLDER;
    strcpy(pack1.data,dest);
    printf("Dest is %s\n", dest);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack1, sizeof(request));
    if(send(clientSocket, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(clientSocket);
        return NULL;
    }
    char* paths[] = {arg, NULL};
    file_system = fts_open(paths, FTS_COMFOLLOW|FTS_NOCHDIR,&compare);
    file_dir pack;

    if (NULL != file_system)
    {
        while( (node = fts_read(file_system)) != NULL)
        {
            switch (node->fts_info) 
            {
                case FTS_D :
                case FTS_F :
                case FTS_SL:
                    if(node->fts_info==FTS_F)
                    {
                        pack.isFile=1;
                    }
                    else
                    {
                        pack.isFile=0;
                    }
                    strcpy(pack.path,node->fts_path);
                    strcpy(pack.name,node->fts_name);
                     printf("Fts Path: %s\n", pack.path);   
                    // if file path has .wav then break
                    if(strstr(node->fts_name, ".wav") != NULL){
                        break;
                    }
                    if(pack.isFile==1)
                    {
                        FILE* file = fopen(node->fts_path, "r");
                        if(file==NULL){
                            printf("File not found");
                            send_err_to_ns(clientSocket,"File not found");
                            close(clientSocket);
                            return NULL;
                        }
                       //get the size of file
                        fseek(file, 0, SEEK_END);
                        long fsize = ftell(file);
                        char data[fsize+1];
                        memset(data, 0, fsize+1);
                        fseek(file, 0, SEEK_SET);
                        //read the file
                        fread(data, 1, fsize, file);
                        strcpy(pack.data, data);
                        fclose(file);
                        char buffer[sizeof(file_dir)];
                        memset(buffer, 0, sizeof(file_dir));
                        memcpy(buffer, &pack, sizeof(file_dir));
                        if(send(clientSocket, buffer, sizeof(file_dir), 0) < 0){
                            printf("Send failed");
                            close(clientSocket);
                            return NULL;
                        }
                        printf("Sent\n");
                        printf("Path: %s\n", pack.path);
                        printf("Data: %s\n", pack.data);
                    }
                    else
                    {
                        char buffer[sizeof(file_dir)];
                        memset(buffer, 0, sizeof(file_dir));
                        memcpy(buffer, &pack, sizeof(file_dir));
                        if(send(clientSocket, buffer, sizeof(file_dir), 0) < 0){
                            printf("Send failed");
                            close(clientSocket);
                            return NULL;
                        }
                        printf("Sent\n");
                        printf("Path: %s\n", pack.path);

                    }
                
                    break;
                default:
                    break;
            }
        }
        fts_close(file_system);
    }
    file_dir pack2;
    memset(&pack2, 0, sizeof(file_dir));
    pack2.isFile=-10;
    char buffer1[sizeof(file_dir)];
    memset(buffer1, 0, sizeof(file_dir));
    memcpy(buffer1, &pack2, sizeof(file_dir));
    
    if(send(clientSocket, buffer1, sizeof(file_dir), 0) < 0){
        printf("Send failed");
        close(clientSocket);
        return NULL;
    }
    printf("Send success\n");
    return 0;
}


// #include "storageserver.c"
 int ns_sockfd;
void* send_ack_to_ns(int Socket,char *msg)
{
    request pack;
    pack.requestType = ACK;
    strcpy(pack.data, msg);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));

    if(send(Socket, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(Socket);
        return NULL;
    }
    return NULL;
}
void* writeAsync(void* args){
    threadArg* t = (threadArg*)args;
    FILE* fp = fopen(t->path, "a");
    char* bufp = t->buffer;
    //write to file in fixed size chunks unti everything is written
    while(bufp[0] != '\0'){
        int sz = MAX_CHUNK_AT_A_TIME < strlen(bufp) ? MAX_CHUNK_AT_A_TIME : strlen(bufp);
        fwrite(bufp, sizeof(char), sz, fp);
        bufp += sizeof(char)*sz;
        sleep(1);
    } 
    char str[MAX_PATH_LENGTH*2] = {0};
    sprintf(str, "%s", t->path);

    request pack;
    pack.requestType = WRITE_ACK;
    strcpy(pack.data, str);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    fclose(fp);
    printf("File written\n");

    int sock;
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    struct hostent *server;
    server = gethostbyname(NS_IP);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(NS_PORT);
    if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        return NULL;
    }
    if(send(sock, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sock);
        return NULL;
    }


    free(t->buffer);
    free(t);
    
    return NULL;
}

int recursive_delete(const char *dir)
{
    DIR *d = opendir(dir);
    size_t path_len = strlen(dir);
    int r = -1;

    if (d)
    {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d)))
        {
            int r2 = -1;
            char *buf;
            size_t len;

            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf)
            {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", dir, p->d_name);

                if (!stat(buf, &statbuf))
                {
                    if (S_ISDIR(statbuf.st_mode))
                    {
                        r2 = recursive_delete(buf);
                    }
                    else
                    {
                        r2 = unlink(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
    {
        r = rmdir(dir);
    }

    return r;
}

void* send_path_to_ns(char* cwd, char* path,int fl)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    // Prepare server address by gethostbyname
    struct hostent *server;
    server = gethostbyname(NS_IP);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(NS_PORT);
    // Connect to naming server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        return NULL;
    }
    request pack;
    pack.requestType = REGISTER_PATH;
    if(fl) {
      if(fl==1)  pack.requestType=REGISTER_PATH_STOP_FOLDER;
        else pack.requestType=REGISTER_PATH_STOP_FILE;
        // char port[10];
   
    strcpy(pack.data,cwd);
    char buff3[sizeof(request)];
    memset(buff3, 0, sizeof(request));
    memcpy(buff3, &pack, sizeof(request));
    if(send(sockfd, buff3, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sockfd);
        return NULL;
    }
        return NULL;

    }
    
    // convert PORT to string
    char port[10];
    sprintf(port, "%d", PORT);
    printf("Port: %s\n", port);
    strcpy(pack.data,port);
    strcat(pack.data, " ");
    strcat(pack.data, cwd);
    strcat(pack.data, "/");
    strcat(pack.data, path);
    printf("Path: %s\n", pack.data);
    // if(fl) strcat(pack.data, " STOP");
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(sockfd, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sockfd);
        return NULL;
    }

   
    // Bind the socket
    return NULL;
}

void* send_unlock_ack_to_ns(char* path)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    // Prepare server address by gethostbyname
    struct hostent *server;
    server = gethostbyname(NS_IP);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(NS_PORT);
    // Connect to naming server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        return NULL;
    }
    request pack;
    pack.requestType = ACK;
    strcpy(pack.data, path);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(sockfd, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sockfd);
        return NULL;
    }

   
    // Bind the socket
    return NULL;
}
void* send_sync_ack_to_ns(int Socket,char *msg)
{
    request pack;
    pack.requestType = ACK;
    strcpy(pack.data, msg);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(Socket, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(Socket);
        return NULL;
    }
    return NULL;
}


// function to handle ns request
void * handle_ns_req(void* arg){
    int clientSocket = *(int*)arg;
    char buffer[sizeof( request)];
    memset(buffer, 0, sizeof(request));
    if(recv(clientSocket, buffer, sizeof( request), 0) < 0){
        printf("Receive failed");
        
        send_err_to_ns(clientSocket,"Receive failed");
        close(clientSocket);
        return NULL;
    }
    request req;
    memset(&req, 0, sizeof(request));
    memcpy(&req, buffer, sizeof( request));
    // printf("Received request from naming server\n");
    // check the request type
    if(req.requestType==CREATEFOLDER)
    {
        // create the folder
        printf("Creating folder\n");
        printf("Folder name: %s\n", req.data);
        if(mkdir(req.data, 0777) < 0){
            printf("Folder creation failed");
            send_err_to_ns(clientSocket,"Folder creation failed");
            close(clientSocket);
            return NULL;
        }
        printf("Folder created successfully\n");
        send_ack_to_ns(clientSocket,"Folder created successfully");
    }
    else if(req.requestType==DELETEFOLDER)
    {
        // delete the folder
        printf("Deleting folder\n");
        printf("Folder name: %s\n", req.data);
       int a= recursive_delete(req.data);
       if(a==-1){
              printf("Folder deletion failed");
              send_err_to_ns(clientSocket,"Folder deletion failed");
              close(clientSocket);
              return NULL;
       }
        printf("Folder deleted successfully\n");
        send_ack_to_ns(clientSocket,"Folder deleted successfully");
        send_unlock_ack_to_ns(req.data);
    }
    else if(req.requestType==CREATEFILE)
    {
        // create the file
        printf("Creating file\n");
        printf("File name: %s\n", req.data);
        FILE* ch=fopen(req.data,"r");
        if(ch!=NULL)
        {
            printf("File already exists");
            send_err_to_ns(clientSocket,"File already exists");
            close(clientSocket);
            return NULL;
        }
        FILE *file = fopen(req.data, "w");

        if(file==NULL){
            printf("File creation failed");
            close(clientSocket);
            send_err_to_ns(clientSocket,"File creation failed");
            return NULL;
        }
        fclose(file);
        printf("File created successfully\n");
        send_ack_to_ns(clientSocket,"File created successfully");
    }
    else if(req.requestType==DELETEFILE)
    {
        // delete the file
        printf("Deleting file\n");
        printf("File name: %s\n", req.data);
        if(unlink(req.data) < 0){
            printf("File deletion failed");
            send_err_to_ns(clientSocket,"File deletion failed");
            close(clientSocket);
            return NULL;
        }
        printf("File deleted successfully\n");
        send_ack_to_ns(clientSocket,"File deleted successfully");
        send_unlock_ack_to_ns(req.data);
    }
    
    else if (req.requestType==COPYFOLDER)
    {
                char ip[20];
                char port[20];
                char path[100];
                char dest[100];
                char *tok = strtok(req.data, " ");
                strcpy(ip, tok);
                tok = strtok(NULL, " ");
                strcpy(port, tok);
                tok = strtok(NULL, " ");   
                strcpy(path, tok);
                tok = strtok(NULL, " ");
                strcpy(dest, tok);
               
                int portno=atoi(port);
               
                printf("Ip %s Port %d Path %s Dest %s\n",ip,portno,path,dest);
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    printf("ERROR opening socket");
                    return NULL;
                }
                struct sockaddr_in serv_addr;
                struct hostent *server;
                server = gethostbyname(ip);
                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host\n");
                    return NULL;
                }
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(portno);
                if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
                    printf("ERROR connecting");
                    return NULL;
                }
                // printf("Connected to naming server\n");
                // printf("Calling fts_search\n");
                fts_search(path,sockfd,dest);
    }
    else if(req.requestType==COPYFILE)
    {
         char ip[20];
              char port[20];
                char path[100];
                char dest[100];
                char *tok = strtok(req.data, " ");
                strcpy(ip, tok);
                tok = strtok(NULL, " ");
                strcpy(port, tok);
                tok = strtok(NULL, " ");   
                strcpy(path, tok);
                tok = strtok(NULL, " ");
                strcpy(dest, tok);
                printf("dest: %s\n", dest);
                int portno=atoi(port);
                printf("IP: %s\n", ip);
                printf("Port: %d\n", portno);
                printf("Path: %s\n", path);
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    printf("ERROR opening socket");
                    return NULL;
                }
                struct sockaddr_in serv_addr;
                struct hostent *server;
                server = gethostbyname(ip);
                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host\n");
                    return NULL;
                }
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(portno);
                if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
                    printf("ERROR connecting");
                    return NULL;
                }
                request pack;
                pack.requestType = PASTEFILE;
                strcpy(pack.data, dest);
                char buffer[sizeof(request)];
                memset(buffer, 0, sizeof(request));
                memcpy(buffer, &pack, sizeof(request));
                if(send(sockfd, buffer, sizeof(request), 0) < 0){
                    printf("Send failed");
                    close(sockfd);
                    return NULL;
                }
                file_dir pack2;
                memset(&pack2, 0, sizeof(file_dir));
                pack2.isFile=1;
                strcpy(pack2.path,path);
                FILE* file = fopen(path, "r");
                if(file==NULL){
                    printf("File not found");
                    send_err_to_ns(clientSocket,"File not found");
                    close(clientSocket);
                    return NULL;
                }
                fseek(file, 0, SEEK_END);
                long fsize = ftell(file);
                char data[fsize+1];
                memset(data, 0, fsize+1);
                fseek(file, 0, SEEK_SET);
                fread(data, 1, fsize, file);
                strcpy(pack2.data, data);
                char buffer1[sizeof(file_dir)];
                memset(buffer1, 0, sizeof(file_dir));
                memcpy(buffer1, &pack2, sizeof(file_dir));
                if(send(sockfd, buffer1, sizeof(file_dir), 0) < 0){
                    printf("Send failed");
                    close(sockfd);
                    return NULL;
                }
                printf("Sent\n");



    }
    
   free(arg);
return NULL;
}

// function to start   and bind the socket to the port for listening to the naming server  
void* NS_listener(void* arg){
      struct sockaddr_in serv_addr;
     
        // Create socket
    ns_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ns_sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    // Prepare server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    setsockopt(ns_sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    
    // Bind the socket
    if (bind(ns_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Bind failed");
        close(ns_sockfd);
        return NULL;
    }
    printf("Bind to port %d successful...\n", PORT);
    // listen to the naming server
    if(listen(ns_sockfd, MAX_CLIENTS) < 0){
        printf("Listen failed");
        close(ns_sockfd);
        return NULL;
    } 
    for(;;){
        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);

        int* clientSocket = (int*)malloc(sizeof(int));
        *clientSocket = accept(ns_sockfd, (struct sockaddr *)&clientaddr, (socklen_t*)&len);
        if(*clientSocket < 0){
            printf("Client accept failed");
            switch (errno)
            {
            case EBADF:
                printf("The argument sockfd is not a valid file descriptor.\n");
                break;
            case EFAULT:
                printf("The address pointed to by addr is not a valid part of the process address space.\n");
                break;
            case EINVAL:
                printf("The socket is not accepting connections.\n");
                break;
            case ENOTSOCK:
                printf("The file descriptor sockfd does not refer to a socket.\n");
                break;
            case EOPNOTSUPP:
                printf("The socket is not of type that supports the accept() operation.\n");
                break;
            case EWOULDBLOCK:
                printf("The socket is marked non-blocking and no connections are present to be accepted.\n");
                break;
            case ECONNABORTED:
                printf("A connection has been aborted.\n");
                break;
            case EINTR:
                printf("The system call was interrupted by a signal that was caught.\n");
                break;
            case EPROTO:
                printf("Protocol error.\n");
                break;
            case EPERM:
                printf("Firewall rules forbid connection.\n");
                break;
            default:
                printf("Unknown error\n");
                break;
            }
            
            close(ns_sockfd);
            return NULL;
        }

        printf("Client connected on socket %d\n", *clientSocket);

        // handle the client request
        pthread_t tid;
        pthread_create(&tid, NULL, handle_ns_req, clientSocket);
        
    }

}
void* send_reg_path_to_ns(char * path)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    // Prepare server address by gethostbyname
    struct hostent *server;
    server = gethostbyname(NS_IP);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        return NULL;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(NS_PORT);
    // Connect to naming server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting");
        return NULL;
    }
    request pack;
    pack.requestType = REGISTER_PATH;
    strcpy(pack.data, path);
    char buffer[sizeof(request)];
    memset(buffer, 0, sizeof(request));
    memcpy(buffer, &pack, sizeof(request));
    if(send(sockfd, buffer, sizeof(request), 0) < 0){
        printf("Send failed");
        close(sockfd);
        return NULL;
    }

   
    
    return NULL;
}
void get_permissions(mode_t mode, char *perm)
{
    strcpy(perm, "----------");
    if (S_ISDIR(mode))
        perm[0] = 'd';
    else if (S_ISLNK(mode))
        perm[0] = 'l';
    else if (S_ISCHR(mode))
        perm[0] = 'c';
    else if (S_ISBLK(mode))
        perm[0] = 'b';
    else if (S_ISFIFO(mode))
        perm[0] = 'p';
    else if (S_ISSOCK(mode))
        perm[0] = 's';

    if (mode & S_IRUSR)
        perm[1] = 'r';
    if (mode & S_IWUSR)
        perm[2] = 'w';
    if (mode & S_IXUSR)
        perm[3] = 'x';
    if (mode & S_IRGRP)
        perm[4] = 'r';
    if (mode & S_IWGRP)
        perm[5] = 'w';
    if (mode & S_IXGRP)
        perm[6] = 'x';
    if (mode & S_IROTH)
        perm[7] = 'r';
    if (mode & S_IWOTH)
        perm[8] = 'w';
    if (mode & S_IXOTH)
        perm[9] = 'x';
}

// function to handle client request
void* handle_client_req(void* arg)
{
    int clientSocket = *(int*)arg;
    char buffer[sizeof( request)];
    memset(buffer, 0, sizeof(request));
    if(recv(clientSocket, buffer, sizeof(request), 0) < 0){
        printf("Receive failed");
        send_err_to_ns(clientSocket,"Receive failed");
        close(clientSocket);
        return NULL;
    }
    request req;
    memset(&req, 0, sizeof( request));
    memcpy(&req, buffer, sizeof( request));
    printf("Received request from client\n");
    printf("\nRequest type: %d, Request data: %s\n", req.requestType, req.data);
    // check the request type
    if(req.requestType==READ){
        char command[MAX_STRUCT_LENGTH];
        memset(command, 0, MAX_STRUCT_LENGTH);
        snprintf(command, MAX_STRUCT_LENGTH, "%d %s", req.requestType, req.data);

        // read the file

        char *filename = req.data;
        FILE *file = fopen(filename, "r+");
        if(file==NULL){
            int errnum = errno;
            if(errnum==ENOENT){
                printf("File not found");
                send_err_to_ns(clientSocket,"File not found");
                close(clientSocket);
                return NULL;
            }
            else if(errnum==EACCES){
                printf("Permission denied");
                send_err_to_ns(clientSocket,"Permission denied");
                close(clientSocket);
                return NULL;
            }
            else if(errnum==EISDIR){
                printf("Is a directory");
                send_err_to_ns(clientSocket,"Is a directory");
                close(clientSocket);
                return NULL;
            }
            else{
                printf("File read failed");
                send_err_to_ns(clientSocket,"File read failed");
                close(clientSocket);
                return NULL;
            }
        }
        char data[MAX_STRUCT_LENGTH];
        memset(data, 0, MAX_STRUCT_LENGTH);
        while(fgets(data, 10, file) != NULL){
            request pack;
            pack.requestType = READ;
            strcpy(pack.data, data);
            printf("Data: %s\n", pack.data);
            char buffer[sizeof(request)];
            memset(buffer, 0, sizeof(request));
            memcpy(buffer, &pack, sizeof(request));
            if(send(clientSocket, buffer, sizeof(request), 0) < 0){
                printf("Send failed");
                close(clientSocket);
                return NULL;
            }
            memset(data, 0, MAX_STRUCT_LENGTH);
        }
        // free(filedata);
        printf("File data sent successfully\n");

        

        send_ack_to_ns(clientSocket,"File data sent successfully");
        // send_unlock_ack_to_ns(path);

    }
    else if(req.requestType==WRITEASYNC){
        // write the file
        printf("Writing file\n");
        printf("Data: %s\n", req.data);
        request pack;
        memset(&pack, 0, sizeof(request));
        char* buf = (char*)calloc(MAX_WRITE_BUFFER_SIZE, sizeof(char));
        while(recv(clientSocket, &pack, sizeof(request), 0) > 0){
            if(pack.requestType==STOP){
                break;
            }
            strcat(buf, pack.data);
            memset(&pack, 0, sizeof(request));
        }
        request client_ack;
        memset(&client_ack, 0, sizeof(request));
        client_ack.requestType = req.requestType;
        snprintf(client_ack.data, sizeof(client_ack.data), "WRITEASYNC Started");
        if (send(clientSocket, &client_ack, sizeof(client_ack), 0) < 0) 
            perror("Error sending acknowledgment");
        else
            printf("Acknowledgment sent: %s\n", client_ack.data);

        printf("File being written\n");
        pthread_t p;
        threadArg* t = (threadArg*)malloc(sizeof(threadArg));
        t->buffer = buf;
        memset(t->path, 0 , MAX_PATH_LENGTH);
        strcpy(t->path, req.data);
        pthread_create(&p, NULL, writeAsync, (void*)t);
    }   
        else if (req.requestType==WRITESYNC){
        printf("Writing file\n");
        printf("Data: %s\n", req.data);
        char* tok=strtok(req.data, "\n");
        char filename[strlen(tok)+1];
        strcpy(filename, tok);
        printf("Filename: %s\n", filename);
        
        FILE *file = fopen(filename, "a");
        if(file==NULL){
            printf("File not found");
            send_err_to_ns(clientSocket,"File not found");
            close(clientSocket);
            return NULL;}

        request pack;
        memset(&pack, 0, sizeof(request));
        while(recv(clientSocket, buffer, sizeof(request), 0) > 0){
            memcpy(&pack, buffer, sizeof(request));

            if(pack.requestType == STOP) {
                break;
            }
            
            fprintf(file, "%s", pack.data);
            memset(&pack, 0, sizeof(request));
            memset(buffer, 0, sizeof(request));
        }
        fflush(file);
        fclose(file);
        printf("File written successfully\n");
        // send_ack_to_ns(clientSocket,"File written successfully");
        // send_unlock_ack_to_ns(filename);
        send_write_ack_to_ns(filename);
    }
    else if(req.requestType==STREAM){
          FILE*  file = fopen(req.data, "rb");
          printf("Audio name: %s\n", req.data);
        if(file==NULL){
            printf("File not found");
            send_err_to_ns(clientSocket,"File not found");
            close(clientSocket);
            return NULL;}
        char data[MAX_STRUCT_LENGTH-2];
        memset(data, 0, MAX_STRUCT_LENGTH - 2);
        size_t nread;
        while((nread = fread(data, 1, MAX_STRUCT_LENGTH, file)) > 0){
            request pack;
            pack.requestType = STREAM;
            strcpy(pack.data, data);
            char buffer[sizeof(request)];
            // memset(buffer, 0, sizeof(request));
            memcpy(buffer, &pack, sizeof(request));
            if(send(clientSocket, data, sizeof(data), 0) < 0){
                printf("Send failed");
                close(clientSocket);
                return NULL;
            }
            // memset(data, 0, MAX_STRUCT_LENGTH);
        }
        printf("Audio File sent successfully\n");
        send_ack_to_ns(clientSocket,"Audio File sent successfully");
        // send_unlock_ack_to_ns(path);
        
    } 
    else if (req.requestType==INFO){
        char file_path[MAX_STRUCT_LENGTH];
        memset(file_path, 0, MAX_STRUCT_LENGTH);
        strcpy(file_path, req.data);
        printf("File path: %s\n", file_path);
        struct stat file_stat;
        if(stat(file_path, &file_stat) < 0){
           int errnum = errno;
           if(errnum==ENOENT){
               printf("File not found");
               send_err_to_ns(clientSocket,"File not found");
               close(clientSocket);
               return NULL;
           }
              else if(errnum==EACCES){
                printf("Permission denied");
                send_err_to_ns(clientSocket,"Permission denied");
                close(clientSocket);
                return NULL;
            }
            else if(errnum==EISDIR){
                printf("Is a directory");
                send_err_to_ns(clientSocket,"Is a directory");
                close(clientSocket);
                return NULL;
            }
            else{
                printf("File info failed");
                send_err_to_ns(clientSocket,"File info failed");
                close(clientSocket);
                return NULL;
            }
            return NULL;
        }
        printf("Sending file info to client\n");
        request pack;
        pack.requestType = INFO;
        char data[MAX_STRUCT_LENGTH];
        memset(data, 0, MAX_STRUCT_LENGTH);
        char permissions[11];      // File permission string
        char atime[20], mtime[20]; // Strings for time
        get_permissions(file_stat.st_mode, permissions);
        strftime(atime, sizeof(atime), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_atime));
        strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
        snprintf(data, MAX_STRUCT_LENGTH, "Permissions: %s, st_nlink %ld , st_uid:%ld , st_gid:%ld , st_size:%ld , atime:%s , mtime:%s",
                 permissions,
                 (long)file_stat.st_nlink,
                 (long)file_stat.st_uid,
                 (long)file_stat.st_gid,
                 (long)file_stat.st_size,
                 atime,
                 mtime);
        printf("File stat: %s\n", data);
        strcpy(pack.data, data);
        char buffer[sizeof(request)];
        memset(buffer, 0, sizeof(request));
        memcpy(buffer, &pack, sizeof(request));
        if(send(clientSocket, buffer, sizeof(request), 0) < 0){
            printf("Send failed");
            send_err_to_ns(clientSocket,"Send failed");
            close(clientSocket);
            return NULL;
        }
        printf("File info sent successfully\n");
        send_ack_to_ns(clientSocket,"File info sent successfully");
        // send_unlock_ack_to_ns(file_path);

    }
    else if(req.requestType==PASTEFOLDER)
    {
        // get the current directory
        char cwd[MAX_PATH_LENGTH];
        char src[MAX_PATH_LENGTH];
        int cnt=0;
        memset(cwd, 0, MAX_PATH_LENGTH);
        getcwd(cwd, MAX_PATH_LENGTH);
        printf("req.data: %s\n", req.data);
        //hange the 
        // if(chdir(req.data) < 0){
        //     printf("Directory change failed");
        //     send_err_to_ns(clientSocket,"Directory change failed");
        //     close(clientSocket);
        //     return NULL;
        // }
        file_dir pack;
        memset(&pack, 0, sizeof(request));
        char buff2[sizeof(file_dir)];
        memset(buff2, 0, sizeof(file_dir));
        int c;
        while((c = recv(clientSocket, buff2, sizeof(file_dir), 0)) > 0){
            memcpy(&pack, buff2, sizeof(buff2));
            int fl=0;
            char fname[MAX_PATH_LENGTH] = {0};
            if (cnt==0)
            {
                strcpy(src,pack.path);
                printf("src recieved: %s\n", src);
                cnt++;
            }
            if(pack.isFile == -10) {
                printf("Folder pasted successfully\n");
                // printf("src: %s\n", src);
                send_path_to_ns(src, req.data,1);
                break;
            }
            if(pack.isFile==1)
            {
                char filename[MAX_PATH_LENGTH];
                memset(filename, 0, MAX_PATH_LENGTH);
                strcpy(filename,req.data);
                strcat(filename,"/");
                strcat(filename,pack.path);
                printf("File name: %s\n", filename);
                FILE* file = fopen(filename, "w");
                if(file==NULL){
                    printf("File not found");
                    send_err_to_ns(clientSocket,"File not found");
                    close(clientSocket);
                    return NULL;
                }
                fwrite(pack.data, 1, strlen(pack.data), file);
                fclose(file);
                send_path_to_ns(req.data, pack.path,0);
                // send_reg_path_to_ns(pack.path);
            }
            else
            {   
                char filename[MAX_PATH_LENGTH];
                memset(filename, 0, MAX_PATH_LENGTH);
                strcpy(filename,req.data);
                strcat(filename,"/");
                strcat(filename,pack.path);
                printf("Folder name: %s\n", filename);
                if(mkdir(filename, 0777) < 0){
                    if(errno==EEXIST){
                        printf("\nFolder already exists");
                        // send_err_to_ns(clientSocket,"Folder already exists");
                        rmdir(filename);
                        mkdir(filename, 0777);
                        send_path_to_ns(req.data, pack.path,0);
                        // close(clientSocket);
                        // return NULL;
                    }
                    // printf("Folder creation failed");
                    else if(errno==EACCES){
                        printf("Permission denied");
                        send_err_to_ns(clientSocket,"Permission denied");
                        close(clientSocket);
                        return NULL;
                    }
                    else{
                        printf("Folder creation failed");
                        send_err_to_ns(clientSocket,"Folder creation failed");
                        close(clientSocket);
                        return NULL;
                    }
                   
                    return NULL;
                }
                // if(fl){
                //     printf("Folder already ghanta\n");
                //     rmdir(fname);
                //     mkdir(fname, 0777);
                // }
                else send_path_to_ns(req.data, pack.path,0);
            }
            memset(&pack, 0, sizeof(request));
            memset(buff2, 0, sizeof(request));
    }
        printf("Done\n");
    }
    else if(req.requestType==PASTEFILE)
    {
        char cwd[MAX_PATH_LENGTH];
        memset(cwd, 0, MAX_PATH_LENGTH);
        getcwd(cwd, MAX_PATH_LENGTH);
        //hange the 
        printf("req.data: %s\n", req.data);
        // if(chdir(req.data) < 0){
        //     printf("Directory change failed");
        //     send_err_to_ns(clientSocket,"Directory change failed");
        //     close(clientSocket);
        //     return NULL;
        // }
        file_dir pack;
        memset(&pack, 0, sizeof(request));
        char buff2[sizeof(file_dir)];
        memset(buff2, 0, sizeof(file_dir));
        if(recv(clientSocket, buff2, sizeof(file_dir), 0) < 0){
            printf("Receive failed");
            send_err_to_ns(clientSocket,"Receive failed");
            close(clientSocket);
            return NULL;
        }
        memcpy(&pack, buff2, sizeof(file_dir));
       
    
        char filename[MAX_PATH_LENGTH];
        memset(filename, 0, MAX_PATH_LENGTH);
        for(int i=strlen(pack.path)-1; i>=0; i--){
            if(pack.path[i]!='/'){
                char temp[2];
                temp[0] = pack.path[i];
                temp[1] = '\0';
                strcat(filename, temp);
            }
            else{
                break;
            }
        }
        // reverse the filename
        char temp[MAX_PATH_LENGTH];
        memset(temp, 0, MAX_PATH_LENGTH);
        for(int i=0;i<strlen(filename);i++){
            temp[i] = filename[strlen(filename)-1-i];
        }
        char temp2[MAX_PATH_LENGTH];
        memset(temp2, 0, MAX_PATH_LENGTH);
        strcpy(temp2, req.data);
        strcat(temp2, "/");
        strcat(temp2, temp);
      FILE* file=  fopen(temp2, "w");
        fprintf(file, "%s", pack.data);
        fclose(file);
        send_path_to_ns(req.data, temp,0);
        send_path_to_ns(pack.path, req.data,2);

    //    if(chdir(cwd)<0){
    //        printf("Directory change failed");
    //        send_err_to_ns(clientSocket,"Directory change failed");
    //        close(clientSocket);
    //        return NULL;}
        printf("File pasted successfully\n");
    }
    close(clientSocket);
    free(arg);
    return NULL;
}
// function to listen the client request
void * Client_listner(void * arg)
{
    struct sockaddr_in serv_addr;
    int sockfd;
    // Create socketF
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("ERROR opening socket");
        return NULL;
    }
    // Prepare server address
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(CLIENT_PORT);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    // Bind the socket
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Bind failed");
        close(sockfd);
        return NULL;
    }
    printf("Bind to port %d successful...\n", CLIENT_PORT);
    // listen to the naming server
    if(listen(sockfd, MAX_CLIENTS) < 0){
        printf("Listen failed");
        close(sockfd);
        return NULL;
    }
    for(;;){
        struct sockaddr_in clientaddr;
        int len = sizeof(clientaddr);

        int* clientSocket = (int*)malloc(sizeof(int));
        *clientSocket = accept(sockfd, (struct sockaddr *)&clientaddr, (socklen_t*)&len);
        if(*clientSocket < 0){
            printf("Client accept failed");
            free(clientSocket);
            // close(sockfd);
            // return NULL;
        }
        printf("New client connected\n");
        // handle the client request
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client_req, clientSocket);
        
    }
}
