

/*
1. 建立tcp连接  
2. 在tcp连接，socket的基础上，发送http协议请求 
3. 服务器在tcp链接socket，返回http协议response
==> a. www.baidu.com 通过DNS请求获得其ip地址 b. tcp连接这个ip地址：端口 c. 发送http协议

阻塞与非阻塞区别：
对于服务器而言，
    如果某个socket是阻塞状态，当我们read(socket)，若socket没有数据，整个线程/程序就会一直挂起，一直等到IO过来
    如果某个socket是非阻塞状态，read(socket)，虽然没有数据，也会立马返回， 线程/程序就不会挂起
所以做服务器时一般选择非阻塞的IO
*/
  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h> 

#include <netdb.h>
#include <fcntl.h>

#define BUFFER_SIZE         4096
#define HTTP_VERSION        "HTTP/1.1"
#define CONNETION_TYPE      "Connection: close\r\n"

// 获得域名的ip地址
char *host_to_ip(const char *hostname) {

    struct hostent *host_entry = gethostbyname(hostname); //查一下struct hosten

    // 点分十进制, 14.215.177.39
    // 通过inet_ntoa将 unsigned char --> char *
    // 0x12121212 --> "18.18.18.18"
    if(host_entry) {
        //hostname -> ip
        return inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
    }

    return NULL;

}

// 实现TCP连接服务器
int http_create_socket(char *ip) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80); //默认http协议的端口号为80
    sin.sin_addr.s_addr = inet_addr(ip); //ip-->hostname

    if(0 != connect(sockfd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in))) {
        return -1;
    }
    
    fcntl(sockfd, F_SETFL, O_NONBLOCK); //设置为非阻塞
    return sockfd;
}

// 发送http协议格式的请求给服务器， 服务器会返回response

// 如www.github.com/Xiaomostream，hostname: github.com; resource: /Xiaomostream
char *http_send_request(const char *hostname, const char *resource) {
    char *ip = host_to_ip(hostname);
    int sockfd = http_create_socket(ip);

    char buffer[BUFFER_SIZE] = {0}; //buffer里面的数据格式为http协议格式
    sprintf(buffer,
"GET %s %s\r\n\
Host: %s\r\n\
%s\r\n\
\r\n", 
    resource, HTTP_VERSION,
    hostname, 
    CONNETION_TYPE
    );
    send(sockfd, buffer, strlen(buffer), 0); //发送给web服务器http协议格式的buffer数据

    //recv();  由于IO是非阻塞的，空数据会立马返回， 需要使用select检测监听网络IO是否为空数据, 只要集合中有一个非空就会返回非空
    
    //select检测， 网络IO里面有没有可读的数据？
    fd_set fdread;  //可以看作是由0/1组成的数组，1表示第几位有数据，这样就可以建立映射关系
    FD_ZERO(&fdread); //先对fdset置空
    FD_SET(sockfd, &fdread);

    struct timeval tv;
    tv.tv_sec = 5; //多少轮回一次
    tv.tv_usec = 0;

    char *result = malloc(sizeof(int));
    memset(result, 0, sizeof(int));
    while(1) {
        /*select(maxfd+1, &rret, &wset, &eset, &tv)
          返回IO数， maxfd表示最大可读集合，最大的fd是多少;
          rset: 一个可读的集合, 关注哪写IO可读; wset: 一个可写的结合，关注哪些IO可写
          eset: 判断哪个IO出错; tv: 表示多长时间轮回一次，即多长遍历一次所有的IO
        */
        int selection = select(sockfd+1, &fdread, NULL, NULL, &tv ); //sockfd+1 表示多开一位数组
        if(!selection || !FD_ISSET(sockfd, &fdread)) { //
            break;
        } else {
            memset(buffer, 0, BUFFER_SIZE);
            int len = recv(sockfd, buffer, BUFFER_SIZE, 0); //这里有可能出现一次recv读不完一条数据，可能需要多次recv读
            if(len == 0) { //disconnect
                break;
            }

            result = realloc(result, (strlen(result) + len + 1) * sizeof(char));
            strncat(result, buffer, len);
        }
    }
    return result;
}

int main(int argc, char *argv[]) {
    if(argc < 3) return -1;
    //printf("%s\n", host_to_ip(argv[1]));
    char *response = http_send_request(argv[1], argv[2]);
    printf("response: %s\n", response);
    free(response);
    return 0;
}