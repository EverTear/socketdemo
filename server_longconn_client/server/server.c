#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT            8234
#define BUFFER_SIZE     1024
#define WAIT_MAX        10

static int serverfd = -1;

void handle_signal(int signo){
    if(signo == SIGINT || signo == SIGTERM){
        printf("Signal %d captured, closing socket...\n", signo);
        if(serverfd >= 0){
            close(serverfd);
        }
        exit(0);
    }
}

int main(){
    int connfd = -1;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);
    unsigned char buffer[BUFFER_SIZE] = {0};
    int ret = 0;
    ssize_t recv_ret = 0, send_ret = 0;
    
    // Register the signal process function
    signal(SIGINT, handle_signal);

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

    // Accept a client connection
    while(1){
        connfd = accept(serverfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (connfd < 0){
            perror("Accept failed");
            goto end;
        }
        printf("Client %d connected\n", connfd);

        while(1){
            // Handle client request
            recv_ret = recv(connfd, buffer, BUFFER_SIZE, 0);
            if(recv_ret == 0){
                //recv_ret == 0 means the client has closed the connection
                // Close the connection
                close(connfd);
                printf("connection %d closed\n", connfd);
                break;
            }else if(recv_ret < 0){
                //recv_ret < 0 indicating a connection problem
                perror("bad connection");
                // Close the connection
                close(connfd);
                printf("connection %d closed\n", connfd);
                break;
            }
            // otherwise, recv_ret represents the length of the data actually read
            
            log_data(stdout, buffer, recv_ret);

            // just send what we receive
            send_ret = send(connfd, buffer, recv_ret, 0);
            if(send_ret != recv_ret){
                printf("send error\n");
                goto end;
            }
        }
    }

end:
    if(connfd >= 0){
        close(connfd);
    }
    if(serverfd >= 0){
        close(serverfd);
    }
    return 0;
}