// 使用reactor来做kvstore的网络框架

#include <stdio.h>
#include <string.h>
/*
msg: request message
length: length of request message
response: need to send
@return:  length
*/
int kvs_protocol(char *msg, int length, char *response) {
    printf("recv %d : %s\n", length, msg);
    memcpy(response, msg, length);
    return strlen(response);
}


