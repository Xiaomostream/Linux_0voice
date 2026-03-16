#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <vector>

#define PORT 10000
#define IP_ADDRESS "192.168.1.19"
#define MESSAGE_SIZE 1024
#define LOOP_NUM 20

int64_t get_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t milliseconds =
        tv.tv_sec * 1000LL + tv.tv_usec / 1000; // 将秒和微秒转换为毫秒
    return milliseconds;
}

// ./rtt_client 192.168.1.19 8080
// ./rtt_client 114.215.169.66 10000
int main(int argc, char *argv[]) {
    char *ip_addr = (char *)IP_ADDRESS;
    int port = PORT;
    int loop_num = LOOP_NUM;
    std::vector<int64_t> rtt_vec;

    if (argc >= 2)
        ip_addr = argv[1];
    if (argc >= 3)
        port = atoi(argv[2]);
    if (argc >= 4)
        loop_num = atoi(argv[3]);
    
    printf("ip:%s, port:%d, loop_num:%d\n", ip_addr, port, loop_num);    

    

    for (int i = 0; i < loop_num; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            printf("Failed to create socket.\n");
            return -1;
        }
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0) {
            printf("Invalid address/ Address not supported.\n");
            return -1;
        }

        if (connect(sock, (struct sockaddr *)&server_addr,
                    sizeof(server_addr)) == -1) {
            printf("Error i:%d connecting to server: %s\n", i, strerror(errno));
            return -1;
        }

        char message[MESSAGE_SIZE];
        memset(message, 'a', MESSAGE_SIZE);

        int64_t start_time = get_ms();
        if (send(sock, message, strlen(message), 0) == -1) {
            printf("Failed to send message.\n");
            return -1;
        }
        char buffer[MESSAGE_SIZE] = {0};

        // 等待接收服务器发送回来的数据
        int n = 0;
        while (1) {
            n = recv(sock, buffer, MESSAGE_SIZE - 1, 0);
            if (n > 0) {
                break;
            } else {
                printf("recv failed\n");
                break;
            }
        }
        // 计算RTT
        int64_t rtt = get_ms() - start_time;
        rtt_vec.push_back(rtt);
        close(sock);
    }
    int64_t total_rtt = 0;
    int64_t min_rtt = rtt_vec[0];
    int64_t max_rtt = rtt_vec[0];
    for (int i = 0; i < loop_num; i++) {
        printf("rtt[%d]: %ldms\n", i, rtt_vec[i]);
        total_rtt += rtt_vec[i]; // 计算总的rtt，方便后续取平均
        if (rtt_vec[i] < min_rtt)
            min_rtt = rtt_vec[i];
        if (rtt_vec[i] > max_rtt)
            max_rtt = rtt_vec[i];
    }
    printf("rtt avg:%ldms, min:%ldms, max:%ldms\n",
           int64_t(total_rtt / loop_num), min_rtt, max_rtt);

    return 0;
}
