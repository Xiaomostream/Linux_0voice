/*
TCP的服务器
1. 基础部门， 网络编程
2. 并发服务器：
    1)一请求一线程
    2)IO多路复用， epoll
3. TCP服务器百万连接

服务器：前台迎宾 --> listen; 服务员 --> clientfd 

多个客户端，如何区分哪个客户端发送的？
    sockfd解决不了，需要借助应用协议。

随着客户端越来越多，比如100我，不适合使用一请求以线程的方式， posix thread 8M . 1G内存 --> 可以开128个线程
*/ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define BUFFER_LENGTH       1024
void *client_routine(void *arg) {
    int clientfd = *(int *)arg;

    while(1) {
        char buffer[BUFFER_LENGTH] = {0};
        int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
        if(len < 0) { 
            //阻塞态，返回-1，通常需要设为非阻塞，但是一请求一线程的方式可以设置为阻塞
            // if(errno == EAGIN || errno == EWOULDBLOCK) {
                
            // }
            close(clientfd);
            break;
        } else if(len == 0) { // disconnect
            close(clientfd);
            break;
        } else {
            printf("Recv: %s, %d byte(s)\n", buffer, len);
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Param Error!\n");
        return -1;
    }
    int port = atoi(argv[1]);
    // 聘请一个迎宾的前台，充当listen
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // 前台与地址addr绑定，开始正式工作 --> listen
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return -2;
    }
    
    // 前台最多只能接待五位顾客
    if(listen(sockfd, 5) < 0) {
        perror("listen");
        return -3;
    } 

    // 一请求一线程
    while(1) {
        //来人时，前台小姐姐sockfd为客户介绍一个新的内部服务员clientfd
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(struct sockaddr_in));
        socklen_t client_len = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_routine, &clientfd);
    }
    return 0;
}
