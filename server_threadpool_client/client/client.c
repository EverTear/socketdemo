#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_IP       "127.0.0.1" // Server IP address
#define SERVER_PORT     8234        // Server port
#define BUFFER_SIZE     1024        // Buffer size for communication
#define THREAD_NUM      32          // Number of threads
#define SEND_LOOP       10          // Send data loop count
#define LOOP_COUNT      200         // Loop count

static unsigned char message[] = {0x01, 0x02, 0x03, 0x04, 0x05};
static struct sockaddr_in server_address = {0};

void* communicate(void* arg){
    int connfd = -1;
    unsigned char* buffer = NULL;
    int ret = 0;
    size_t i = 0, j = 0;

    printf("thread %lu sending\n", (unsigned long)pthread_self());

    buffer = (unsigned char*)calloc(BUFFER_SIZE, 1);
    if(NULL == buffer){
        perror("calloc error");
        goto end;
    }

    for(i = 0; i < LOOP_COUNT; ++i){
        // Create socket
        connfd = socket(PF_INET, SOCK_STREAM, 0);
        if(connfd < 0){
            perror("Socket creation failed");
            goto end;
        }

        // Connect to the server
        ret = connect(connfd, (struct sockaddr *)&server_address, sizeof(server_address));
        if(ret < 0){
            perror("Connection to server failed");
            goto end;
        }

        for(j = 0; j < SEND_LOOP; ++j){
            // Send data to the server
            ret = send(connfd, message, sizeof(message), 0);
            if(ret != sizeof(message)){
                perror("send error");
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
        }
        close(connfd);
    }

end:
    if(connfd >= 0){
        close(connfd);
    }
    if(buffer != NULL){
        free(buffer);
    }
    pthread_exit(NULL);
}

int main() {
    int ret = 0;
    pthread_t threads[THREAD_NUM] = {0};
    size_t i = 0;
    unsigned long long start_time = 0, end_time = 0;

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);

    // Convert IPv4 address from text to binary form
    ret = inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr);
    if(ret <= 0){
        printf("Invalid address / Address not supported\n");
        return -1;
    }

    start_time = get_current_time_ms();

    for(i = 0; i < THREAD_NUM; ++i){
        pthread_create(&threads[i], NULL, communicate, NULL);
        // sleep(1);
    }

    for(i = 0; i < THREAD_NUM; ++i){
        pthread_join(threads[i], NULL);
    }

    end_time = get_current_time_ms();

    printf("Time elapsed: %llu ms\n", end_time - start_time);

    return 0;
}