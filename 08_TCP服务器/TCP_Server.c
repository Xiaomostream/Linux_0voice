#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/epoll.h>

#define BUFFER_LENGTH 1024
#define EPOLL_SIZE 1024

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
    int epfd = epoll_create(1);
    struct epoll_event events[EPOLL_SIZE] = {0};

    struct epoll_event ev;
    ev.data.fd = sockfd;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);

    while(1) {
        int nready = epoll_wait(epfd, events, EPOLL_SIZE, 5);
        if(nready == -1) continue;

        for (int i = 0; i < nready; i ++) {
            if(events[i].data.fd == sockfd) {
                struct sockaddr_in client_addr;
                memset(&client_addr, 0, sizeof(struct sockaddr_in));
                socklen_t client_len = sizeof(client_addr);
                int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len); 

                ev.data.fd = clientfd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
            }
            else {
                int clientfd = events[i].data.fd;
                char buffer[BUFFER_LENGTH] = {0};
                int len = recv(clientfd, buffer, BUFFER_LENGTH, 0);
                if(len == 0) {
                    //disconnect
                    close(clientfd);
                    ev.data.fd = clientfd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    break;
                } else if(len < 0) {
                    close(clientfd);
                    ev.data.fd = clientfd;
                    ev.events = EPOLLIN | EPOLLET;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    break;
                } else {
                    printf("Recv: %s %d byte(s)\n", buffer, len);
                }
            }
        }

    }
    return 0;
}