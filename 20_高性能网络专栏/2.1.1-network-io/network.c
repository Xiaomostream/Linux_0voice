// netstat -anop | grep 2500
// listen后，可以被连接，并且会产生新连接状态
// IO与TCP连接状态, 通过accept分配, 所以fd通常和TCP的生命周期是非常相似的。
// 网络IO：一个客户端对应一个连接：fd
#include <asm-generic/siginfo.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <poll.h>

void *clientfd_thread(void *arg) {
    int clientfd = *(int *)arg;
    while(1) {
        char buffer[1024] = {0};
        int count = recv(clientfd, buffer, 1024, 0);
        // 客户端断开连接，recv返回0
        if(count == 0) { //disconnect;
            printf("client %d disconnect\n", clientfd);
            close(clientfd);
            break;
        }

        // parser，对客户端发来的信息进行解析

        printf("Recv: %s\n", buffer);
        count = send(clientfd, buffer, count, 0);
        printf("Send: %d\n", count);
    }
}
int main(int argc, char* argv[]) {
    if(argc < 2) {
        printf("param error!");
        return -1;
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    servaddr.sin_port = htons(atoi(argv[1]));
    if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))) {
        printf("bind failed: %s\n", strerror(errno));
        return -1;
    }
    listen(sockfd, 10);
    printf("listen %d finshed\n", sockfd);
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
#if 0
    int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len); //等待客户端连接进来, 如果连接成功，给其分配一个fd

    printf("客户端 %d 连接成功！\n", clientfd);
    char buffer[1024]={0};
    int count = recv(clientfd, buffer, 1024, 0);
    printf("Recv: %s\n", buffer);
    count = send(clientfd, buffer, count, 0);

    printf("Send: %d\n", count);
#elif 0
    // 第一个不接收数据，后续连接的客户端都不能send, 原因是第一个客户端accept后，等待recv，后续的客户端此时再连接，也不能accept
    // --> 多个io如何监听数据
    while(1) {
        int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
        printf("客户端 %d 连接成功！\n", clientfd);
        char buffer[1024]={0};
        int count = recv(clientfd, buffer, 1024, 0);
        printf("Recv: %s\n", buffer);
        count = send(clientfd, buffer, count, 0);
        printf("Send: %d\n", count);
    }
#elif 0
    // 一请求一线程 任务参数是 clientfd，任务是接收发送数据
    // 一个线程服务于一个客户端，线程之间各自独立
    // 缺点：
    while(1) {
        printf("accept\n");
        int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
        printf("客户端 %d 连接成功！\n", clientfd);
        
        pthread_t thid;
        pthread_create(&thid, NULL, clientfd_thread, &clientfd);
    }
#elif 0
    fd_set rfds, rset;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    int maxfd = sockfd;
    while(1) {
        rset = rfds;
        int nready = select(maxfd+1, &rset, NULL, NULL, NULL); //有多少个bit位设为1了，也就是这三个集合[r,w,e]有多少个fd可读可写
        // maxfd+1最大的fd范围，timeout表示等待时长，多久询问一次，如果是NULL，代表阻塞态，一直等待新的fd
        if(FD_ISSET(sockfd, &rset)) { //判断sockfd是否在rset集合中，也就是判断sockfd是否可读
            // accept
            int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
            printf("客户端 %d 连接成功！\n", clientfd);
            FD_SET(clientfd, &rfds); 
            if(maxfd < clientfd) maxfd = clientfd; //加入了新的clientfd，判断maxfd是否更新
        }
        // recv
        for (int i = sockfd+1; i <= maxfd; i ++ ) { //处理的是fd
            if(FD_ISSET(i, &rset)) {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                // 客户端断开连接，recv返回0
                if(count == 0) { //disconnect;
                    printf("client %d disconnect\n", i);
                    close(i);
                    FD_CLR(i, &rfds);
                    continue;
                }
                // parser，对客户端发来的信息进行解析
                printf("Recv: %s\n", buffer);
                count = send(i, buffer, count, 0);
                printf("Send: %d\n", count);
            }
        }

    }
#elif 0
    struct pollfd fds[1024] = {0};
    fds[sockfd].fd = sockfd;
    fds[sockfd].events = POLL_IN;

    int maxfd = sockfd;
    while(1) {
        int nready = poll(fds, maxfd+1, -1);
        if(fds[sockfd].revents & POLL_IN) {
            int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
            printf("客户端 %d 连接成功！\n", clientfd);
            fds[clientfd].fd = clientfd;
            fds[clientfd].events = POLL_IN;
            if(clientfd > maxfd) maxfd = clientfd;
        }

        for (int i = sockfd+1; i <= maxfd; i ++ ) {
            if(fds[i].revents & POLL_IN) {
                char buffer[1024] = {0};
                int count = recv(i, buffer, 1024, 0);
                // 客户端断开连接，recv返回0
                if(count == 0) { //disconnect;
                    printf("client %d disconnect\n", i);
                    close(i);
                    fds[i].fd = -1;
                    fds[i].events = 0;
                    continue;
                }
                // parser，对客户端发来的信息进行解析
                printf("Recv: %s\n", buffer);
                count = send(i, buffer, count, 0);
                printf("Send: %d\n", count);
            }
        }
    }
#elif 1
    int epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sockfd;
    
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while(1) {
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, -1);

        for (int i = 0; i < nready; i ++ ) {
            int connfd = events[i].data.fd;

            if(connfd == sockfd) {
                int clientfd = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
                printf("客户端 %d 连接成功！\n", clientfd);
                ev.events = EPOLLIN;
                ev.data.fd = clientfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
            } else if(events[i].events & EPOLLIN){
                char buffer[1024] = {0};
                int count = recv(connfd, buffer, 1024, 0);
                if(count == 0) {
                    printf("client %d disconnect\n", connfd);
                    close(connfd);
                    // epoll删除connfd，可以不传入第五个参数event的配置, 传入NULL即可。
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                    continue;
                }
                // parser，对客户端发来的信息进行解析
                printf("Recv: %s\n", buffer);
                count = send(connfd, buffer, count, 0);
                printf("Send: %d\n", count);
            }
        }
    }

#endif
    getchar();
    printf("exit!\n");
    return 0;
}