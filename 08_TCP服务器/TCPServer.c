/*
一. TCP的服务器
1. 基础部门， 网络编程
2. 并发服务器：
    1)一请求一线程
    2)IO多路复用， epoll
3. TCP服务器百万连接

二. 服务器：前台迎宾 --> listen; 服务员 --> clientfd 

多个客户端，如何区分哪个客户端发送的？
    sockfd解决不了，需要借助应用协议。

随着客户端越来越多，比如100w，不适合使用一请求以线程的方式， posix thread 8M . 1G内存 --> 可以开128个线程

三. epoll是什么？
比如一个服务器(小区)，里面有很多个客户端，每个客户端都在服务器有连接(socket)，每个IO相当于小区的住户收发快递
epoll是来管理这些IO，能够检测到哪个IO有数据，从而把这个提示返回给应用层，便于实现业务逻辑。这个epoll相当于小区的快递员，来检测哪个住户有快递了。
1. epoll_create() ：聘请一个快递员
2. epoll_ctl() : 添加/关闭一个IO; update一个IO从A到B
3. epoll_wait() : 多久时间去一次小区

四. 关于IO有没有数据？
1. 一种是检测是否有数据     水平触发， 可触发多次，可分多次读完
2. 一种是检测数据从无到有   边沿触发， 只触发一次，一次性读完
五. 面试时：epoll水平触发与边沿触发说清楚
    开发时：要注意sockfd、clientfd等 IO的变化有没有在epoll的集合里
sockfd, epoll

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

#include <sys/epoll.h>

#define BUFFER_LENGTH       1024
#define EPOLL_SIZE          1024

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

#if 0
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
#else 
    int epfd = epoll_create(1); //这个参数只要 >0 即可
    // IO事件，快递箱子 
    struct epoll_event events[EPOLL_SIZE] = {0};

    // 把listen_fd(sockfd)加入到epoll管理
    struct epoll_event ev;
    ev.events = EPOLLIN; //EPOLLIN只关注读，EPOLLOUT只关注写
    ev.data.fd = sockfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while(1) {
        int nready = epoll_wait(epfd, events, EPOLL_SIZE, 5); //(快递员， 快递盒子， 盒子多大， 多久时间去一次); -1表示只要有时间就去一次
        // nready表示有多少个IO事件
        if(nready == -1) continue;

        for (int i = 0; i < nready; i ++ ) {
            // listenfd与clientfd处理方式不一样
            // listenfd只能通过accept处理，clientfd可以通过recv处理
            if(events[i].data.fd == sockfd) { //listenfd
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_len = sizeof(client_addr);
                int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
                
                ev.events = EPOLLIN || EPOLLET; //使用边沿触发，如果有数据，一次性读完
                ev.data.fd = clientfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else {
                int clientfd = events[i].data.fd;
                char buffer[BUFFER_LENGTH] = {0};
                int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
                if(len < 0) { //阻塞
                    close(clientfd); //关闭IO后，记得使用epoll_ctl，delete掉
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    break;
                } else if(len == 0) { //disconnect
                    close(clientfd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    break;
                } else {
                    printf("Recv: %s %d byte(s)\n", buffer, len);
                }
            }
        }
    }
#endif
    return 0;
}
