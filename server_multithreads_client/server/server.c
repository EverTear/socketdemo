#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define PORT            8234
#define BUFFER_SIZE     1024
#define WAIT_MAX        1024

typedef struct handle_conn_arg_st{
    int connfd;
}handle_conn_arg_t;

static int serverfd = -1;

void signal_handler(int signo){
    if(signo == SIGINT || signo == SIGTERM){
        printf("Signal %d captured, closing socket...\n", signo);
        if(serverfd >= 0){
            close(serverfd);
        }
        exit(0);
    }
}

void* handle_conn(void* arg){
    handle_conn_arg_t* para = (handle_conn_arg_t*)arg;
    int ret = 0;
    unsigned char* buffer = NULL;
    unsigned int send_len = 0;

    buffer = (unsigned char*)calloc(BUFFER_SIZE, 1);
    if(buffer == NULL){
        perror("calloc");
        goto end;
    }
    
    // Handle client request
    while(1){
        ret = recv(para->connfd, buffer, BUFFER_SIZE, 0);
        if(ret == 0){
            //ret == 0 means the client has closed the connection
            printf("connection %d closed\n", para->connfd);
            close(para->connfd);
            break;
        }else if(ret < 0){
            //ret < 0 indicating a connection problem
            printf("Bad connection %d: %d\n", para->connfd, errno);
            goto end;
        }
        // otherwise, ret represents the length of the data actually read
        // log_data(stdout, buffer, ret);
        // just send what we recevie
        
        send_len = send(para->connfd, buffer, ret, 0);
        if(send_len != ret){
            perror("send error");
            goto end;
        }

        continue;
    }

end:
    if(para->connfd >= 0){
        close(para->connfd);
    }

    if(buffer != NULL){
        free(buffer);
    }

    free(arg);
    
    pthread_exit(NULL);
}

int main(){
    int connfd = -1;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);
    int ret = 0;
    pthread_t thread = {0};
    handle_conn_arg_t* thread_arg = NULL;

    // Register the signal process function
    signal(SIGINT, signal_handler);

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
    ret = bind(serverfd, (struct sockaddr *)&address, sizeof(address));
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
        connfd = accept(serverfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (connfd < 0){
            perror("Accept failed");
            goto end;
        }
        printf("Client %d connected\n", connfd);

        thread_arg = (handle_conn_arg_t*)calloc(sizeof(handle_conn_arg_t), 1);
        if(NULL == thread_arg){
            printf("calloc error\n");
            goto end;
        }
        thread_arg->connfd = connfd;
        pthread_create(&thread, NULL, handle_conn, (void*)thread_arg);
        pthread_detach(thread);
    }

end:
    if(serverfd >= 0){
        close(serverfd);
    }

    return 0;
}