#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_IP       "127.0.0.1" // Server IP address
#define SERVER_PORT     8234      // Server port
#define BUFFER_SIZE     1024      // Buffer size for communication

int main(){
    int connfd = -1;
    struct sockaddr_in server_address = {0};
    unsigned char buffer[BUFFER_SIZE] = {0};
    unsigned char message[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    int ret = 0;

    // Create socket
    connfd = socket(PF_INET, SOCK_STREAM, 0);
    if (connfd < 0) {
        printf("Socket creation failed\n");
        goto fail;
    }

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    ret = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
    if(ret <= 0){
        printf("Invalid address / Address not supported.\n");
        goto fail;
    }

    // Connect to the server
    ret = connect(connfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if(ret < 0){
        printf("Connection to server failed\n");
        goto fail;
    }
    printf("Connected to the server\n");

    // Send data to the server
    ret = send(connfd, message, sizeof(message), 0);
    if(ret != sizeof(message)){
        printf("send error\n");
        goto fail;
    }
    printf("Message sent to server\n");

    // Receive response from the server
    printf("Response from server:\n");
    while(1){
        ret = recv(connfd, buffer, BUFFER_SIZE, 0);
        if(ret == 0){
            //ret == 0 means the client has closed the connection
            break;
        }else if(ret < 0){
            //ret < 0 indicating a connection problem
            printf("bad connection: %d\n", errno);
            break;
        }
        // otherwise, ret represents the length of the data actually read
        
        log_data(stdout, buffer, ret);

        if(ret == BUFFER_SIZE){
            // there is still data that has not been read
            continue;
        }
        // all data has been read
        break;
    }

    // Close the socket
    close(connfd);
    printf("Connection closed\n");

    return 0;

fail:
    if(connfd >= 0){
        close(connfd);
    }
    return -1;
}