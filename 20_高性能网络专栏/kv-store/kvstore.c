// 使用reactor来做kvstore的网络框架

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kvstore.h"
/*
 *msg: request message
 *length: length of request message
 *response: need to send
 *@return:  length
*/
int kvs_protocol(char *msg, int length, char *response) {
    printf("recv %d : %s\n", length, msg);
    memcpy(response, msg, length);
    return strlen(response);
}

extern int reactor_start(unsigned int port, msg_handler handler);
extern int Ntyco_start(unsigned int port, msg_handler handler);
extern int proactor_start(unsigned int port, msg_handler handler);
// port, kvs_protocol
int main(int argc, char *argv[]) {

    if(argc != 2) return -1;
    unsigned int port = atoi(argv[1]);
    //reactor_start(port, kvs_protocol);
    //Ntyco_start(port, kvs_protocol);
    proactor_start(port, kvs_protocol);
    return 0;
}