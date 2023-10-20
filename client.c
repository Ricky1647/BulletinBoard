#include "hw.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <poll.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include<string.h>
#include <fcntl.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define BUFFER_SIZE 512

typedef struct {
    char* ip; // server's ip
    unsigned short port; // server's port
    int conn_fd; // fd to talk with server
    char buf[BUFFER_SIZE]; // data sent by/to server
    size_t buf_len ; // bytes used by buf
} client;

client cli;
static void init_client(char** argv);

void handleBoard();

int main(int argc, char** argv){
    
    // Parse args.
    if(argc!=3){
        ERR_EXIT("usage: [ip] [port]");
    }

    // Handling connection
    init_client(argv);
    fprintf(stderr, "connect to %s %d\n", cli.ip, cli.port);
    printf("==============================\nWelcome to CSIE Bulletin board\n==============================\n");
    fflush(stdout);
    char buf[RECORD_NUM*sizeof(record)]="";
    handleBoard();
    printf("==============================\n");
    fflush(stdout);
    int sent = 0;
    while(1){
        printf("Please enter your command (post/pull/exit): ");
        fflush(stdout);
        fflush(stdout);
        scanf("%s",cli.buf);
        cli.buf[4] = '\n';
        write(cli.conn_fd,cli.buf,4);
        cli.buf[4] = '\0';
        if(strcmp(cli.buf,"exit")==0){
            read(cli.conn_fd,cli.buf,cli.buf_len);
            exit(0);    
        }
        else if(strcmp(cli.buf,"pull")==0){
            printf("==============================\n");
            fflush(stdout);
            handleBoard();
            printf("==============================\n");
            fflush(stdout);
        }
        else if(strcmp(cli.buf,"post")==0){
            int tmptmp = 0;
            if(read(cli.conn_fd,cli.buf,cli.buf_len)!=1){
                printf("%s",cli.buf);
                continue;
            }
            printf("FROM: ");
            record* post_record = malloc(sizeof(record));
            scanf("%s",&post_record->From);
            memset(post_record+strlen(post_record->From),'\0',FROM_LEN-strlen(post_record->From));
            printf("CONTENT:\n");
            fflush(stdout);
            scanf("%s",&post_record->Content);
            memset(post_record+sizeof(post_record->From)+strlen(post_record->Content),'\0',CONTENT_LEN-strlen(post_record->Content));
            write(cli.conn_fd,post_record,sizeof(record));
        }

    }
 
}
void handleBoard(){
    int sent = 0;
    record* allBoard = malloc(sizeof(record)*RECORD_NUM);
    if((sent = read(cli.conn_fd,allBoard,sizeof(record)*RECORD_NUM))!=1){
        int index = sent/sizeof(record);
        for(int i = 0; i < index; i++){
            printf("FROM: ");
            printf("%.*s\n",FROM_LEN,allBoard[i].From);
            printf("CONTENT:\n");
            printf("%.*s\n",CONTENT_LEN,allBoard[i].Content);
        }
        fflush(stdout);
    }
    free(allBoard);
}

static void init_client(char** argv){
    
    cli.ip = argv[1];
    cli.buf_len = BUFFER_SIZE;
    if(atoi(argv[2])==0 || atoi(argv[2])>65536){
        ERR_EXIT("Invalid port");
    }
    cli.port=(unsigned short)atoi(argv[2]);

    struct sockaddr_in servaddr;
    cli.conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(cli.conn_fd<0){
        ERR_EXIT("socket");
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(cli.port);

    if(inet_pton(AF_INET, cli.ip, &servaddr.sin_addr)<=0){
        ERR_EXIT("Invalid IP");
    }

    if(connect(cli.conn_fd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
        ERR_EXIT("connect");
    }

    return;
}
