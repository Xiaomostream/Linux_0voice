#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define MESSAGE_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;

    int opt = 1;
    int addrlen = sizeof(address);
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to create socket.\n");
        return -1;
    }
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("Failed to set socket options.\n");
        return -1;
    }
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        printf("Failed to bind socket.\n");
        return -1;
    }
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
    if (listen(server_fd, 3) == -1) {
        printf("Failed to listen on socket.\n");
        return -1;
    }
    printf("%s(%d)\n", __FUNCTION__, __LINE__);
   while(1)
   {
       char buffer[MESSAGE_SIZE] = {0};
       int n = 0;
        printf("accept into\n");
       //等待客户端连接
       if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t*)&addrlen))==-1)
       {
           printf("Failed to accept connection.\n");
           continue;
       }
      printf("new fd = %d\n", new_socket); 
      //接收来自客户端的数据并发送回复
      n = recv(new_socket , buffer , MESSAGE_SIZE-1 , 0);
      printf("recv  n = %d\n",  n);
      send(new_socket , buffer , strlen(buffer) , 0);
      printf("send n = %d\n",  n);
      close(new_socket);
   }

   return 0;
}
