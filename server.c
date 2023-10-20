#include "hw.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include<string.h>
#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define BUFFER_SIZE 512

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[BUFFER_SIZE];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
    int last;
    struct flock* recordlock;
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list
int last = RECORD_NUM-1;
// initailize a server, exit for error
static void init_server(unsigned short port);

// initailize a request instance
static void init_request(request* reqP);

// free resources used by a request instance

static void free_request(request* reqP);


struct flock* createWlock(int last);

int outputBoard(int index,int Boardfd,record* dump);
void setWriteLock(int fd_num , int filed);
int recordTable[RECORD_NUM] = {0};

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        ERR_EXIT("usage: [port]");
        exit(1);
    }
    //write(STDOUT_FILENO,"AAA",3);

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    int file_fd;  // fd for file that we open for readin
    //O_NONBLOCKING
    char* erro_mess = "[Error] Maximum posting limit exceeded\n";
    record dump;
    memset(&dump,'\0',sizeof(record));
    int Bulletin  = open(RECORD_PATH,O_RDWR | O_CREAT | O_NONBLOCK);
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);
    fcntl(svr.listen_fd, F_SETFL, O_NONBLOCK);
    fd_set master_set,readingset;
    FD_ZERO(& master_set);
    FD_SET(svr.listen_fd,&master_set);
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int local_max_fd = svr.listen_fd+1;

    while (1) {
        readingset = master_set;
        int activity = 0;
        if((activity = select(local_max_fd+1,&readingset,NULL,NULL,&timeout))>0){
            for(int i = 3; i < local_max_fd+1; i++){
                if(FD_ISSET(i,&readingset)){
                    if( i == svr.listen_fd ){
                        clilen = sizeof(cliaddr);
                        conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                        if (conn_fd < 0) {
                            if (errno == EINTR || errno == EAGAIN) continue;  // try again
                            if (errno == ENFILE) {
                                (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                                continue;
                            }
                            ERR_EXIT("accept");
                        }
                        requestP[conn_fd].conn_fd = conn_fd;
                        requestP[conn_fd].recordlock = NULL;
                        strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                        fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
                        fcntl(requestP[conn_fd].conn_fd , F_SETFL, O_NONBLOCK);
                        FD_SET(requestP[conn_fd].conn_fd,&master_set);


                        int lockpost = outputBoard(conn_fd,Bulletin,&dump);
                        setWriteLock(local_max_fd,Bulletin);
                        if(lockpost > 0){
                            printf("[Warning] Try to access locked post - %d\n",lockpost);
                            fflush(stdout);
                        }
                        local_max_fd++;

                    }
                    else{
                        char inputbuffer[RECORD_NUM*sizeof(record)] = "";
                        int socketlen = 0;
                        socketlen=read(requestP[i].conn_fd,inputbuffer,RECORD_NUM*sizeof(record));
                        if(socketlen == sizeof(record)){
                            record* postrecord = (record*)inputbuffer;
                            pwrite(Bulletin,postrecord,sizeof(record),requestP[i].last*sizeof(record));
                            printf("[Log] Receive post from %.*s\n",FROM_LEN,postrecord->From);
                            fflush(stdout);
                            requestP[i].recordlock->l_type = F_UNLCK;
                            if (fcntl(Bulletin, F_SETLK, requestP[i].recordlock) == -1) {
                                perror("Failed to release lock");
                            }
                            recordTable[requestP[i].last] = 0;
                            free(requestP[i].recordlock);
                            requestP[i].recordlock = NULL;
                        }
                        else if(strcmp(inputbuffer,"pull")==0){
                            int lockpost = outputBoard(i,Bulletin,&dump);
                            setWriteLock(local_max_fd,Bulletin);
                            if(lockpost > 0){
                                printf("[Warning] Try to access locked post - %d\n",lockpost);
                                fflush(stdout);
                            }
                        }
                        else if(strcmp(inputbuffer,"exit")==0){
                            write(requestP[i].conn_fd,"ACK\n",4);
                            close(requestP[i].conn_fd);
                            FD_CLR(requestP[i].conn_fd,&master_set);
                        }
                        else if (strcmp(inputbuffer,"post")==0){
                            int key = 1;
                            int not_found_unlock = last;
                            last = (last+1) % RECORD_NUM;
                            requestP[i].recordlock = createWlock(last);

                            while (fcntl(Bulletin, F_SETLK, requestP[i].recordlock) == -1 || recordTable[last] == 1) {
                                last = (last+1) % RECORD_NUM;
                                if(last == not_found_unlock){
                                    key = 0;
                                    write(requestP[i].conn_fd,erro_mess,strlen(erro_mess));
                                    free(requestP[i].recordlock);
                                    requestP[i].recordlock = NULL;
                                    break;
                                }
                                free(requestP[i].recordlock);
                                requestP[i].recordlock = createWlock(last);
                            }
                            fprintf(stderr,"last locked %d\n",last);
                            recordTable[last] = 1;
                            requestP[i].last = last;
                            if(key == 1)
                                write(requestP[i].conn_fd,"A",1);
                        }

                    }
                }
            }
        }

    }
    free(requestP);

    return 0;
}

void setWriteLock(int fd_num, int filed){
    for(int i = 4; i < fd_num; i++){
        if(requestP[i].recordlock != NULL && requestP[i].recordlock->l_type == F_WRLCK){
            //requestP[i].recordlock->l_type = F_WRLCK;
            if (fcntl(filed, F_SETLK, requestP[i].recordlock) == -1) {
                perror("Failed to release lock");
            }
        }
    }
}

struct flock* createWlock(int last){
    struct flock* recordlock = malloc(sizeof(struct flock));
    memset(recordlock, '\0', sizeof(struct flock));
    recordlock->l_type = F_WRLCK;
    recordlock->l_whence = SEEK_SET;
    recordlock->l_start = last*(FROM_LEN+CONTENT_LEN);
    recordlock->l_len = FROM_LEN+CONTENT_LEN;
    return recordlock;
}
inline int outputBoard(int fd,int Boardfd,record* dump){
    struct flock readlock ;
    record allbullet[RECORD_NUM];
    int index = 0;
    int lockpost = 0;
    for(int i = 0; i < RECORD_NUM; i++){
        memset(&readlock, '\0', sizeof(struct flock));
        readlock.l_type = F_RDLCK;
        readlock.l_whence = SEEK_SET;
        readlock.l_start = sizeof(record)*i;
        readlock.l_len = sizeof(record);
        if(fcntl(Boardfd, F_SETLK, &readlock) == -1 || recordTable[i] == 1){
            lockpost++;

            continue;
        }
        if((pread(Boardfd,&allbullet[index],sizeof(record),sizeof(record)*i))>0 && !(memcmp(allbullet[index].From,dump->From,FROM_LEN)==0)){
            index++;
        }
        readlock.l_type = F_UNLCK;
        if (fcntl(Boardfd, F_SETLK, &readlock) == -1) {
            perror("Failed to release lock");
        }   
    }

    if(index == 0){
        write(requestP[fd].conn_fd,"A",1);
    }
    else{
        write(requestP[fd].conn_fd,allbullet,index*sizeof(record));
    }
    return lockpost;
}
static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    init_request(reqP);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}


