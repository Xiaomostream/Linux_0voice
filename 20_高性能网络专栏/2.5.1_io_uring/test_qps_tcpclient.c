#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

typedef struct test_context_s {
    char serverip[16];
    int port;
    int pthreadnum;
    int connection;
    int requestion;

#if 1
    int failed;
#endif
} test_context_t;

int connect_tcpserver(const char *ip, int port) {

    int connfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in tcpserver_addr;
    memset(&tcpserver_addr, 0, sizeof(struct sockaddr_in));

    tcpserver_addr.sin_family = AF_INET;
    tcpserver_addr.sin_addr.s_addr = inet_addr(ip); //点分十进制ip转化为unsigned 32(4*8)
    tcpserver_addr.sin_port = htons(port);

    int ret = connect(connfd, (struct sockaddr*)&tcpserver_addr, sizeof(struct sockaddr_in));
    if(ret) {
        perror("connect\n");
        return -1;
    }
    return connfd;
}

#define TEST_MESSAGE    "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghijklmnopqrstuvwxyz\r\n"
#define RBUFFER_LENGTH  120
int send_recv_tcppkt(int fd) {
    int ret = send(fd, TEST_MESSAGE, strlen(TEST_MESSAGE), 0);
    if(ret < 0) {
        exit(1);
    }
    char rbuffer[RBUFFER_LENGTH] = {0};
    ret = recv(fd, rbuffer, RBUFFER_LENGTH, 0);
    if(ret <= 0) {
        exit(1);
    }
    if(strcmp(rbuffer, TEST_MESSAGE) != 0) {
        printf("failed: '%s' != '%s'\n", rbuffer, TEST_MESSAGE);
        return -1;
    }
    return 0;
}

static void *test_qps_entry(void *arg) {
    test_context_t *pctx = (test_context_t*)arg;
    int connfd = connect_tcpserver(pctx->serverip, pctx->port);
    if(connfd < 0) {
        printf("connection_tcpserver failed\n");
        return NULL;
    }

    int count = pctx->requestion / pctx->pthreadnum;
    int i = 0;
    int res;
    while(i++ < count) {
        res = send_recv_tcppkt(connfd);
        if(res != 0) {
            printf("send_recv_tcpkt failed\n");
            pctx->failed ++;
        }
    }
    return NULL;
}

// ./test_qps_tcpclient -s 127.0.0.1 -p 2048 -t 50 -c 100 -n 10000
// ./test_qps_tcpclient -s 127.0.0.1 -p 8888 -t 50 -c 100 -n 1000000
//                      -ip地址 -端口 -线程数量 -连接数量 -请求数量
int main(int argc, char *argv[]) {
    int ret = 0;
    test_context_t ctx = {0};

    int opt;
    while((opt = getopt(argc, argv, "s:p:t:c:n:?")) != -1) {
        switch (opt) {
            case 's':
                printf("-s :%s\n", optarg);
                strcpy(ctx.serverip, optarg);
                break;
            case 'p':
                printf("-p :%s\n", optarg);
                ctx.port = atoi(optarg);
                break;
            case 't':
                printf("-t :%s\n", optarg);
                ctx.pthreadnum = atoi(optarg);
                break;
            case 'c':
                printf("-c :%s\n", optarg);
                ctx.connection = atoi(optarg);
                break;
            case 'n':
                printf("-n :%s\n", optarg);
                ctx.requestion = atoi(optarg);
                break;
        }
    }

    pthread_t *ptid = malloc(ctx.pthreadnum * sizeof(pthread_t)); //在堆上定义
    //pthread_t ptid[10] = {0}; //在栈上定义 

    struct timeval tv_begin;
    gettimeofday(&tv_begin, NULL);
    for (int i = 0; i < ctx.pthreadnum; i ++ ) {
        pthread_create(&ptid[i], NULL, test_qps_entry, &ctx);
    }
    for (int i = 0; i < ctx.pthreadnum; i ++ ) {
        pthread_join(ptid[i], NULL);
    }

    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);

    int time_used = TIME_SUB_MS(tv_end, tv_begin);
    printf("success: %d, failed: %d, time_used: %d\n", ctx.requestion-ctx.failed, ctx.failed, time_used);

clean:
    free(ptid);

    return ret;
}

// ./test_qps_tcpclient -s 192.168.66.1 -p 2048 -t 50 -c 100 -n 10000