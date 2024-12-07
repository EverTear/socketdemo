#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT            8234
#define BUFFER_SIZE     1024
#define WAIT_MAX        3

typedef struct handle_conn_arg_st{
    int connfd;
}handle_conn_arg_t;

static int serverfd;

void signal_handler(int signo){
    printf("Signal captured, closing socket...\n");
    close(serverfd);
}

void* handle_conn(void* arg){
    handle_conn_arg_t* para = (handle_conn_arg_t*)arg;
    int ret;
    unsigned char* buffer;

    buffer = (unsigned char*)calloc(BUFFER_SIZE, 1);
    if(buffer == NULL){
        perror("calloc");
        close(para->connfd);
        pthread_exit(NULL);
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
            perror("bad connection");
            close(para->connfd);
            break;
        }
        // otherwise, ret represents the length of the data actually read
        log_data(stdout, buffer, ret);
        // just send what we recevie
        send(para->connfd, buffer, ret, 0);

        continue;
    }

    free(buffer);
    free(arg);
    pthread_exit(NULL);
}

int main(){
    int connfd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int ret;
    pthread_t thread;
    handle_conn_arg_t* thread_arg;

    // Create socket file descriptor
    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if(serverfd == 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    ret = bind(serverfd, (struct sockaddr *)&address, sizeof(address));
    if(ret < 0){
        perror("Bind failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    ret = listen(serverfd, WAIT_MAX);
    if(ret < 0){
        perror("Listen failed");
        close(serverfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d\n", PORT);

    while(1){
        connfd = accept(serverfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (connfd < 0){
            perror("Accept failed");
            close(serverfd);
            exit(EXIT_FAILURE);
        }
        printf("Client connected\n");

        thread_arg = (handle_conn_arg_t*)malloc(sizeof(handle_conn_arg_t));
        if(thread_arg == NULL){
            perror("malloc error");
            close(connfd);
            continue;
        }
        thread_arg->connfd = connfd;
        pthread_create(&thread, NULL, handle_conn, (void*)thread_arg);
        pthread_detach(thread);
    }

    return 0;
}