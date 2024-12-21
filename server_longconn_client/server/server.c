#include "common.h"

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

void signal_handler(int signo){
    printf("Signal captured, closing socket...\n");
    if(serverfd >= 0){
        close(serverfd);
    }
}

int main(){
    int connfd = -1;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);
    unsigned char buffer[BUFFER_SIZE] = {0};
    int ret = 0;
    
    // Register the signal process function
    signal(SIGINT, signal_handler);

    // Create socket file descriptor
    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if(serverfd < 0){
        perror("Socket creation failed");
        goto fail;
    }

    // Configure address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    ret = bind(serverfd, (struct sockaddr *)&address, sizeof(address));
    if(ret < 0){
        perror("Bind failed");
        goto fail;
    }

    // Start listening for incoming connections
    ret = listen(serverfd, WAIT_MAX);
    if(ret < 0){
        perror("Listen failed");
        goto fail;
    }
    printf("Server listening on port %d\n", PORT);

    // Accept a client connection
    while(1){
        connfd = accept(serverfd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (connfd < 0){
            perror("Accept failed");
            goto fail;
        }
        printf("Client %d connected\n", connfd);

        // Handle client request
        while(1){
            ret = recv(connfd, buffer, BUFFER_SIZE, 0);
            if(ret == 0){
                //ret == 0 means the client has closed the connection
                break;
            }else if(ret < 0){
                //ret < 0 indicating a connection problem
                perror("bad connection");
                break;
            }
            // otherwise, ret represents the length of the data actually read
            log_data(stdout, buffer, ret);
            // just send what we recevie
            send(connfd, buffer, ret, 0);

            continue;
        }
        // Close the connection
        close(connfd);
        printf("connection %d closed\n", connfd);
    }

    return 0;

fail:
    if(connfd >= 0){
        close(connfd);
    }
    if(serverfd >= 0){
        close(serverfd);
    }
    return -1;
}