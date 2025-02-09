#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP       "127.0.0.1" // Server IP address
#define SERVER_PORT     8234      // Server port
#define BUFFER_SIZE     1024      // Buffer size for communication

int main(){
    int connfd = -1;
    struct sockaddr_in server_address = {0};
    unsigned char buffer[BUFFER_SIZE] = {0};
    unsigned char message[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    int ret = 0;
    ssize_t sr_ret = 0;

    // Create socket
    connfd = socket(PF_INET, SOCK_STREAM, 0);
    if (connfd < 0) {
        perror("Socket creation failed");
        goto end;
    }

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    ret = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
    if(ret <= 0){
        printf("Invalid address / Address not supported.\n");
        goto end;
    }

    // Connect to the server
    ret = connect(connfd, (struct sockaddr*)&server_address, sizeof(server_address));
    if(ret < 0){
        perror("Connection to server failed");
        goto end;
    }
    printf("Connected to the server\n");

    // Send data to the server
    sr_ret = send(connfd, message, sizeof(message), 0);
    if(sr_ret != sizeof(message)){
        printf("send error\n");
        goto end;
    }
    printf("Message sent to server\n");

    // Receive response from the server
    printf("Response from server:\n");
    
    sr_ret = recv(connfd, buffer, BUFFER_SIZE, 0);
    if(sr_ret == 0){
        //sr_ret == 0 means the server has closed the connection
        goto end;
    }else if(sr_ret < 0){
        //sr_ret < 0 indicating a connection problem
        perror("bad connection");
        goto end;
    }
    // otherwise, sr_ret represents the length of the data actually read
    log_data(stdout, buffer, sr_ret);
    
end:
    // Close the socket
    if(connfd >= 0){
        close(connfd);
        printf("Connection closed\n");
    }
    return 0;
}