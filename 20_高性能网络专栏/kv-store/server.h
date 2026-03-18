#ifndef __SERVER_H__

#define __SERVER_H__
#define BUFFER_LENGTH       1024

#define ENABLE_HTTP         0
#define ENABLE_WEBSOCKET    0
#define ENABLE_KVSTORE      1
typedef int (*RCALLBACK)(int fd);

struct conn {
    // 什么事件执行什么回调函数！
    int fd;
    
    char rbuffer[BUFFER_LENGTH];
    int rlength;

    char wbuffer[BUFFER_LENGTH];
    int wlength;

    // EPOLLOUT
    RCALLBACK send_callback;

    // EPOLLIN
    union{
        RCALLBACK recv_callback;
        RCALLBACK accept_callback;
    } r_action;

    int status; //状态机
};

#if ENABLE_HTTP
int http_request(struct conn *c);
int http_response(struct conn *c);
#endif

#if ENABLE_WEBSOCKET
int ws_request(struct conn *c);
int ws_response(struct conn *c);
#endif

#if ENABLE_KVSTORE
int kvs_request(struct conn *c);
int kvs_response(struct conn *c);
#endif

#endif