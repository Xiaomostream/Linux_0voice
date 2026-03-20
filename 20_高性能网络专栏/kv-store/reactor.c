#include <bits/types/struct_timeval.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include "server.h"

#define CONNECTION_SIZE     1024 // 1024*1024
#define MAX_PORTS           1 //20
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

#if ENABLE_KVSTORE

typedef int (*msg_handler)(char *msg, int length, char *response);

static msg_handler kvs_handler;

int kvs_request(struct conn *c) {
    //printf("recv %d : %s\n", c->rlength, c->rbuffer);

    c->wlength = kvs_handler(c->rbuffer, c->rlength, c->wbuffer);

}
int kvs_response(struct conn *c) {
    
}
#endif

int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);

int epfd = 0; //epfd设为全局变量

struct timeval tbegin;

struct conn conn_list[CONNECTION_SIZE] = {0};


void set_event(int fd, int event, int flag) {
    if(flag) { // non-zero add
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    } else { // zero mod
        struct epoll_event ev;
        ev.events = event;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
    }

}

int event_register(int fd, int event) {
    if(fd < 0) return -1; 
    conn_list[fd].fd = fd;
    conn_list[fd].r_action.recv_callback = recv_cb;
    conn_list[fd].send_callback = send_cb;

    memset(conn_list[fd].rbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].rlength = 0;

    memset(conn_list[fd].wbuffer, 0, BUFFER_LENGTH);
    conn_list[fd].wlength = 0;

    set_event(fd, event, 1);

}
// listen(sockfd) --> EPOLLIN --> accept_cb
int accept_cb(int fd) {
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);

    int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
     
    //printf("Clientfd: %d connected, sockfd: %d\n", clientfd, fd);
    if(clientfd < 0) {
        printf("accept errno: %d\n", errno);
        return -1;
    }
#if 0
    //加入到事件中，设置不同的回调函数
    conn_list[clientfd].fd = clientfd;
    conn_list[clientfd].r_action.recv_callback = recv_cb;
    conn_list[clientfd].send_callback = send_cb;

    //清空buffer，以防被污染
    memset(conn_list[clientfd].rbuffer, 0, BUFFER_LENGTH);
    conn_list[clientfd].rlength = 0;
    memset(conn_list[clientfd].wbuffer, 0, BUFFER_LENGTH);
    conn_list[clientfd].wlength = 0;

    //epoll管理clientfd的EPOLLIN
    set_event(clientfd, EPOLLIN);
#elif 1
    event_register(clientfd, EPOLLIN); //默认是水平触发，| EPOLLET切换为水平触发
#endif
    if((clientfd%1000) == 0) { //每一千条连接打印一次信息
        //struct timeval current;
        //gettimeofday(&current, NULL);

        //int time_used = TIME_SUB_MS(current, tbegin);
        //memcpy(&tbegin, &current, sizeof(struct timeval));
        //time_used: %d, time_used
        printf("Clientfd: %d connected\n", clientfd);
    }
    return 0;
}

int recv_cb(int fd) {
    memset(conn_list[fd].rbuffer, 0, BUFFER_LENGTH);
    int count = recv(fd, conn_list[fd].rbuffer, BUFFER_LENGTH, 0);
    if(count == 0) { //disconnect
        printf("client %d disconnect\n", fd);
        close(fd);

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // unfinished
        return -1;
    } else if(count < 0) { 
        //阻塞的IO为什么会返回-1？原因是errno: 104, Connection reset by peer
        //也就是 wrk，关闭连接时发送了 reset, 连接被重置，我们把他close掉就可。
        printf("client: %d, errno: %d, %s\n", fd, errno, strerror(errno));
        close(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL); // unfinished

        return 0;
    }
    conn_list[fd].rlength = count;
    //printf("Recv: %s\n", conn_list[fd].rbuffer);

#if 0 //echo
    conn_list[fd].wlength = conn_list[fd].rlength;
    memcpy(conn_list[fd].wbuffer, conn_list[fd].rbuffer, conn_list[fd].wlength);
#elif ENABLE_HTTP
    http_request(&conn_list[fd]);
#elif ENABLE_WEBSOCKET
    ws_request(&conn_list[fd]);
#elif ENABLE_KVSTORE
    kvs_request(&conn_list[fd]);
#endif    
    set_event(fd, EPOLLOUT, 0);

    return count;
}

int send_cb(int fd) {

#if ENABLE_HTTP
    http_response(&conn_list[fd]);
#elif ENABLE_WEBSOCKET
    http_response（&conn_list[fd]);
#elif ENABLE_KVSTORE
    kvs_response(&conn_list[fd]);
#endif
    int count = 0;
    // 状态机
#if 0
    if(conn_list[fd].status == 1) {
        //printf("SEND: %s\n", conn_list[fd].wbuffer);
        count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
        set_event(fd, EPOLLOUT, 0);
    } else if(conn_list[fd].status == 2) {
        set_event(fd, EPOLLOUT, 0);
    } else if(conn_list[fd].status == 0) {
        if(conn_list[fd].wlength != 0) {
            count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
        }
        set_event(fd, EPOLLIN, 0);
    }
#else 
    if(conn_list[fd].wlength != 0) {
        count = send(fd, conn_list[fd].wbuffer, conn_list[fd].wlength, 0);
    }
    set_event(fd, EPOLLIN, 0);
#endif
    return count;
}
int r_init_server(unsigned short port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))) {
        printf("bind failed: %s\n", strerror(errno));
    }

    listen(sockfd, 10);
    printf("listen finished: %d\n", sockfd);

    return sockfd;
}

//把reactor的main函数封装为函数接口
int reactor_start(unsigned short port, msg_handler handler) {
    kvs_handler = handler;
    //printf("kvs_handler\n");
    // register : 我们关注 sockfd 这个io 的 EPOLLIN 这个事件, 使用epoll来管理
    epfd = epoll_create(1);

    for (int i = 0; i < MAX_PORTS; i ++ ) {
        int sockfd = r_init_server(port+i);

        conn_list[sockfd].fd = sockfd;
        conn_list[sockfd].r_action.recv_callback = accept_cb;

        set_event(sockfd, EPOLLIN, 1);
    }

    //gettimeofday(&tbegin, NULL);
    while(1) { // 服务器的主事件 mainloop
        struct epoll_event events[1024] = {0};
        int nready = epoll_wait(epfd, events, 1024, 0);

        for (int i = 0; i < nready; i ++ ) {
            int connfd = events[i].data.fd;
            // 判断的是事件，执行其对应的回调函数
            // 读写同时判断，表示IO的读写事件可能并存
            if(events[i].events & EPOLLIN) {  //可读执行recv_cb
                conn_list[connfd].r_action.recv_callback(connfd);
            }
            if(events[i].events & EPOLLOUT) { //可写执行send_cb
                conn_list[connfd].send_callback(connfd);
            }
        }
    }

    getchar();
    return 0;
}