#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_IP       "127.0.0.1" // Server IP address
#define SERVER_PORT     8234      // Server port
#define BUFFER_SIZE     1024      // Buffer size for communication
#define CONN_NUM        32          // Number of connections

static unsigned char message[] = {0x01, 0x02, 0x03, 0x04, 0x05};
static struct sockaddr_in server_address = {0};

void* communicate(void* arg){
    int connfd = -1;
    unsigned char* buffer = NULL;
    int ret = 0;
    size_t i = 0;

    buffer = (unsigned char*)calloc(BUFFER_SIZE, 1);
    if(NULL == buffer){
        printf("calloc error\n");
        goto end;
    }

    // Create socket
    connfd = socket(PF_INET, SOCK_STREAM, 0);
    if(connfd < 0){
        perror("Socket creation failed: ");
        goto end;
    }

    // Connect to the server
    ret = connect(connfd, (struct sockaddr *)&server_address, sizeof(server_address));
    if(ret < 0){
        perror("Connection to server failed: ");
        goto end;
    }
    printf("Created connection: %d\n", connfd);
    printf("Connection %d sending\n", connfd);

    for(i = 0; i < 10; ++i){
        // Send data to the server
        ret = send(connfd, message, sizeof(message), 0);
        if(ret != sizeof(message)){
            perror("send error: ");
            goto end;
        }

        // Receive response from the server
        while(1){
            ret = recv(connfd, buffer, BUFFER_SIZE, 0);
            if(ret == 0){
                //ret == 0 means the client has closed the connection
                break;
            }else if(ret < 0){
                //ret < 0 indicating a connection problem
                printf("Bad connection %d: %d\n", connfd, errno);
                goto end;
            }
            // otherwise, ret represents the length of the data actually read

            // log_data(stdout, buffer, ret);

            if(ret == BUFFER_SIZE){
                // there is still data that has not been read
                continue;
            }
            // all data has been read
            break;
        }
        sleep(1);
    }

end:
    // Close the socket
    if(connfd >= 0){
        close(connfd);
        printf("Connection %d closed\n", connfd);
    }
    if(buffer != NULL){
        free(buffer);
    }
    pthread_exit(NULL);
}

int main() {
    int ret = 0;
    pthread_t threads[CONN_NUM] = {0};
    size_t i = 0;

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    ret = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
    if(ret <= 0){
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    for(i = 0; i < CONN_NUM; ++i){
        pthread_create(&threads[i], NULL, communicate, NULL);
        // sleep(1);
    }

    for(i = 0; i < CONN_NUM; ++i){
        pthread_join(threads[i], NULL);
    }

    return 0;
}