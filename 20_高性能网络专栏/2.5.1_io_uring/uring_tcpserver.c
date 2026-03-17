
#include <netinet/in.h>
#include <stdio.h>
#include <liburing.h>
#include <unistd.h>
#include <string.h>


#define ENTRIES_LENGTH 1024
#define BUFFER_LENGTH 1024

#define EVENT_ACCEPT 0
#define EVENT_READ 1
#define EVENT_WRITE 2
struct conn_info {
    int fd;
    int event;
};
int init_server(unsigned short port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if(-1 == bind(sockfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr))) {
        printf("bind failed: %s\n", strerror(errno));
        return -1;
    }

    listen(sockfd, 10);
    printf("listen finished: %d\n", sockfd);

    return sockfd;
}

int set_event_recv(struct io_uring *ring, int sockfd, void *buf, size_t len, int flags) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info accept_info = {
        .fd = sockfd,
        .event = EVENT_READ,
    };
    io_uring_prep_recv(sqe, sockfd, buf, len, flags);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}

int set_event_send(struct io_uring *ring, int sockfd, void *buf, size_t len, int flags) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info accept_info = {
        .fd = sockfd,
        .event = EVENT_WRITE,
    };
    io_uring_prep_send(sqe, sockfd, buf, len,flags);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}

int set_event_accept(struct io_uring *ring, int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags) {

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    struct conn_info accept_info = {
        .fd = sockfd,
        .event = EVENT_ACCEPT,
    };
    io_uring_prep_accept(sqe, sockfd, addr, addrlen, flags);
    memcpy(&sqe->user_data, &accept_info, sizeof(struct conn_info));
}
int main(int argc, char *argv[]) {
    // if(argc < 2) {
    //     printf("Params error");
    //     return 0;
    // }
    int sockfd = init_server(8888);

    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    
    struct io_uring ring;
    // 构建submit_queue和complete_queue
    io_uring_queue_init_params(ENTRIES_LENGTH, &ring, &params);
    
#if 0
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    accept(sockfd, (struct sockaddr*)&clientaddr, &len);
#else
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    //往ring队列里增加一个节点
    set_event_accept(&ring, sockfd, (struct sockaddr*)&clientaddr, &len, 0);
#endif
    char buffer[BUFFER_LENGTH] = {0};
    while(1) {
        io_uring_submit(&ring);

        // printf("before io_uring_wait\n");
        struct io_uring_cqe *cqe;
        io_uring_wait_cqe(&ring, &cqe);
        // printf("after io_uring_wait\n");

        struct io_uring_cqe *cqes[128];
        int nready = io_uring_peek_batch_cqe(&ring, cqes, 128);
        for (int i = 0; i < nready; i ++ ) {
            // printf("io_uring_peek_batch_cqe\n");

            struct io_uring_cqe *entries = cqes[i];
            struct conn_info result;
            memcpy(&result, &entries->user_data, sizeof(struct conn_info));
            
            if(result.event == EVENT_ACCEPT) {
                set_event_accept(&ring, sockfd, (struct sockaddr*)&clientaddr, &len, 0);
                //printf("set_event_accept\n");

                int connfd = entries->res;
                set_event_recv(&ring, connfd, buffer, BUFFER_LENGTH, 0);
            } else if(result.event == EVENT_READ) {

                int ret = entries->res;
                //printf("set_event_recv: %s ret: %d\n", buffer, ret);

                if(ret == 0) {
                    close(result.fd);
                } else if(ret > 0) {
                     set_event_send(&ring, result.fd, buffer, ret, 0);
                }
                
            } else if(result.event == EVENT_WRITE) {
                int ret = entries->res;
                //printf("set_event_send: %s ret: %d\n", buffer, ret);
                set_event_recv(&ring, result.fd, buffer, BUFFER_LENGTH, 0);
            }
            
        }

        // 记得把cqe从队列里清空
        io_uring_cq_advance(&ring, nready);
    }
    return 0;
}