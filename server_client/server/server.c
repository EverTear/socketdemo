#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT            8234
#define BUFFER_SIZE     1024
#define WAIT_MAX        10

int main(){
    int serverfd = -1, connfd = -1;
    struct sockaddr_in address = {0};
    int addrlen = sizeof(address);
    unsigned char buffer[BUFFER_SIZE] = {0};
    int ret = 0;

    // Create socket file descriptor
    serverfd = socket(PF_INET, SOCK_STREAM, 0);
    if (serverfd < 0) {
        perror("Failed to create socket.\n");
        goto fail;
    }

    // Configure address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address and port
    ret = bind(serverfd, (struct sockaddr*)&address, sizeof(address));
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
    connfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if(connfd < 0){
        perror("Accept failed");
        goto fail;
    }
    printf("Client connected\n");

    // Handle client request
    while(1){
        ret = recv(connfd, buffer, BUFFER_SIZE, 0);
        if(ret == 0){
            //ret == 0 means the client has closed the connection
            break;
        }else if(ret < 0){
            //ret < 0 indicating a connection problem
            perror("bad connection\n");
            break;
        }
        // otherwise, ret represents the length of the data actually read
        log_data(stdout, buffer, ret);
        // just send what we recevie
        send(connfd, buffer, ret, 0);

        if(ret == BUFFER_SIZE){
            // there is still data that has not been read
            continue;
        }
        // all data has been read
        break;
    }

    // Close the sockets
    close(connfd);
    close(serverfd);
    return 0;

fail:
    if(connfd >= 0){
        close(connfd);
    }
    if(serverfd >= 0){
        close(connfd);
    }
    return -1;
}