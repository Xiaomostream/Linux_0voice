#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#define MAX_MSG_LENGTH 1024
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

int send_msg(int connfd, char *msg, int length) {
    int res = send(connfd, msg, length, 0);
    if(res < 0) {
        perror("send");
        exit(1);
    }
    return res;
}
int recv_msg(int connfd, char *msg, int length) {
    int res = recv(connfd, msg, length, 0);
    if(res < 0) {
        perror("send");
        exit(1);
    }
    return res;
}


void testcase(int connfd, char *msg, char *pattern, char *casename) {
    if(!msg || !pattern || !casename) return;

    send_msg(connfd, msg, strlen(msg));

    char result[MAX_MSG_LENGTH] = {0};
    recv_msg(connfd, result, MAX_MSG_LENGTH);

    if(strcmp(result, pattern) == 0) {
        printf("==> PASS -> %s\n", casename);
    } else {
        printf("==> FAILED -> %s, '%s' != '%s'\n", casename, result, pattern);
        exit(1);
    }
}

int connect_tcpserver(const char *ip, unsigned short port) {
    int connfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if(0 != connect(connfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        perror("connect");
        return -1;
    }
    return connfd;
}

void array_testcase_10w(int connfd) {
    int count = 1000;
    struct timeval tv_begin;
    gettimeofday(&tv_begin, NULL);
    for (int i = 0; i < count; i ++ ) {
        testcase(connfd, "SET Teacher King", "OK\r\n", "SET-Teacher");
        testcase(connfd, "GET Teacher", "King\r\n", "GET_Teacher");
        testcase(connfd, "MOD Teacher Xiaomo", "OK\r\n", "MOD_Teacher");
        testcase(connfd, "GET Teacher", "Xiaomo\r\n", "GET_Teacher");
        testcase(connfd, "EXIST Teacher", "EXIST\r\n", "EXIST_Teacher");
        testcase(connfd, "DEL Teacher", "OK\r\n", "DEL_Teacher");
        testcase(connfd, "EXIST Teacher", "NOT EXIST\r\n", "EXIST_Teacher");
        testcase(connfd, "GET Teacher", "NOT EXIST\r\n", "GET_Teacher");
        testcase(connfd, "MOD Teacher King", "NOT EXIST\r\n", "MOD_Teacher");
    }
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    printf("array testcase --> time_used: %d, qps: %d\n", time_used, 9*count*1000 / time_used);

}

void rbtree_testcase_10w(int connfd) {
    int count = 1;
    struct timeval tv_begin;
    gettimeofday(&tv_begin, NULL);
    for (int i = 0; i < count; i ++ ) {
        testcase(connfd, "RSET Teacher King", "OK\r\n", "RSET-Teacher");
        testcase(connfd, "RGET Teacher", "King\r\n", "RGET_Teacher");
        testcase(connfd, "RMOD Teacher Xiaomo", "OK\r\n", "RMOD_Teacher");
        testcase(connfd, "RGET Teacher", "Xiaomo\r\n", "RGET_Teacher");
        testcase(connfd, "REXIST Teacher", "EXIST\r\n", "REXIST_Teacher");
        testcase(connfd, "RDEL Teacher", "OK\r\n", "RDEL_Teacher");
        testcase(connfd, "REXIST Teacher", "NOT EXIST\r\n", "REXIST_Teacher");
        testcase(connfd, "RGET Teacher", "NOT EXIST\r\n", "RGET_Teacher");
        testcase(connfd, "RMOD Teacher King", "NOT EXIST\r\n", "RMOD_Teacher");
    }
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);
    int time_used = TIME_SUB_MS(tv_end, tv_begin);

    printf("array testcase --> time_used: %d, qps: %d\n", time_used, 9*count*1000 / time_used);

}


// ./Test_case 192.168.66.130 8888
int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Params error\n");
        return -1;
    }

    char *ip = argv[1];
    unsigned short port = atoi(argv[2]);
    int connfd = connect_tcpserver(ip, port);

    rbtree_testcase_10w(connfd);
    return 0;
}