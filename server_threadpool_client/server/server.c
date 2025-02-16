#include "common.h"
#include "threadpool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define PORT            8234
#define BUFFER_SIZE     1024
#define WAIT_MAX        1024

typedef struct handle_conn_arg_st{
    int connfd;
}handle_conn_arg_t;

static int serverfd = -1;
static cthread_pool_t* threadpool = NULL;

void recycler(){
    if(serverfd >= 0){
        close(serverfd);
    }
    if(NULL != threadpool){
        destroy_threadpool(threadpool);
    }
}

void signal_handler(int signo){
    if(signo == SIGINT || signo == SIGTERM){
        printf("Signal captured, closing socket...\n");
        recycler();
        exit(0);
    }
}

void* handle_conn(void* arg){
    handle_conn_arg_t* para = (handle_conn_arg_t*)arg;
    int ret = 0;
    unsigned char* buffer = NULL;
    unsigned int send_len = 0;

    buffer = (unsigned char*)calloc(BUFFER_SIZE, 1);
    if(NULL == buffer){
        perror("calloc error");
        goto fail;
    }
    
    // Handle client request
    while(1){
        ret = recv(para->connfd, buffer, BUFFER_SIZE, 0);
        if(ret == 0){
            //ret == 0 means the client has closed the connection
            goto fail;
        }else if(ret < 0){
            //ret < 0 indicating a connection problem
            perror("bad connection");
            goto fail;
        }
        // otherwise, ret represents the length of the data actually read
        // log_data(stdout, buffer, ret);
        // just send what we recevie
        send_len = send(para->connfd, buffer, ret, 0);
        if(send_len != ret){
            perror("send error");
            goto fail;
        }
        continue;
    }

    free(buffer);
    free(arg);
    return NULL;

fail:
    if(buffer != NULL){
        free(buffer);
    }
    if(para->connfd >= 0){
        close(para->connfd);
    }
    free(arg);
    return NULL;
}

int main(){
    int connfd = -1;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);
    int ret = 0;
    handle_conn_arg_t* thread_arg = NULL;

    signal(SIGINT, signal_handler);

    threadpool = create_threadpool(128, 128);
    if(NULL == threadpool){
        printf("Create threadpool error\n");
        goto end;
    }

    // Create socket file descriptor
    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if(serverfd < 0){
        perror("Socket creation failed");
        goto end;
    }

    // Configure address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    ret = bind(serverfd, (struct sockaddr*)&address, sizeof(address));
    if(ret < 0){
        perror("Bind failed");
        goto end;
    }

    // Start listening for incoming connections
    ret = listen(serverfd, WAIT_MAX);
    if(ret < 0){
        perror("Listen failed");
        goto end;
    }
    printf("Server listening on port %d\n", PORT);

    while(1){
        connfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (connfd < 0){
            perror("Accept failed");
            goto end;
        }

        thread_arg = (handle_conn_arg_t*)calloc(sizeof(handle_conn_arg_t), 1);
        if(NULL == thread_arg){
            perror("calloc error");
            goto end;
        }
        thread_arg->connfd = connfd;
        add_job(threadpool, handle_conn, (void*)thread_arg);
    }

end:
    recycler();
    return 0;
}