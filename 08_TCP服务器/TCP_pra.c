#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <unistd.h>

#define BUFFER_LENGTH 1024

void *clent_routine(void *arg) {
    int clientfd = *(int *)arg;
    while(1) {
        char buffer[BUFFER_LENGTH] = {0};
        int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
        if(len == 0) {
            //disconnect
            close(clientfd);
            break;
        } else if(len < 0) {
            //阻塞态，返回-1，通常需要设为非阻塞，但是一请求一线程的方式可以设置为阻塞
            // if(errno == EAGIN || errno == EWOULDBLOCK) {
                
            // }
            close(clientfd);
            break;
        } else {
            printf("Recv: %s %d byte(s)\n", buffer, len);
        }
    }
}
int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Param Error\n");
        return -1;
    }

    int port = atoi(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return -2;
    }

    if(listen(sockfd, 5) < 0) {
        perror("listen");
        return -3;
    }

    while(1) {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(client_addr);
        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len); 
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, clent_routine, &clientfd);
    }
    return 0;
}