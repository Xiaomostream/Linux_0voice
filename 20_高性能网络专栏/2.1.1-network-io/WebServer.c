#include "server.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/sendfile.h>

#define WEBSERVER_ROOTDIR   .
// 业务层
int http_request(struct conn *c) {
    //printf("request: %s\n", c->rbuffer);

    memset(c->wbuffer, 0, BUFFER_LENGTH);
    c->wlength = 0;
    c->status = 0;
}

int http_response(struct conn *c) {

#if 1
    // send一个字符串
    c->wlength = sprintf(c->wbuffer, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 82\r\n"
        "Date: Thu, 30 Apr 2025 17:44:32 GMT\r\n\r\n"
        "<html><head><title>0voice.Xiaomo</title></head><body><h1>Xiaomo</h1></body></html>\r\n\r\n"
    );
    return c->wlength;
#elif 0
    // 只发一次，文件太大，可能发不完
    // 发送文件 ==> 读文件转化为字符处理
    int filefd = open("Xiaomostream.html", O_RDONLY);
    // 获取文件大小：1.seek(调到最后) 2.fstat
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    // http_head
    c->wlength = sprintf(c->wbuffer, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: %ld\r\n"
        "Date: Thu, 30 Apr 2025 17:44:32 GMT\r\n\r\n",
        stat_buf.st_size
    );
    // http_body: 从文件尾进行读，读的最大长度为BUFFER_LENGTH-wlength
    int count = read(filefd, c->wbuffer + c->wlength, BUFFER_LENGTH-(c->wlength));
    c->wlength += count;
    // printf("count = %d\n", count);
    close(filefd);

    return c->wlength;
#elif 0
    int filefd = open("Xiaomostream.html", O_RDONLY);
    // 获取文件大小：1.seek(调到最后) 2.fstat
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    if(c->status == 0) {
        c->wlength = sprintf(c->wbuffer, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type： text/html\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: %ld\r\n"
            "Date: Thu, 30 Apr 2025 17:44:32 GMT\r\n\r\n",
            stat_buf.st_size
        );
        c->status = 1;
    } else if(c->status == 1){
        int ret = sendfile(c->fd, filefd, NULL, stat_buf.st_size);
        if(ret == -1) {
            printf("errno: %s", strerror(errno));
        }
        // c->wlength = 0;
        // memset(c->wbuffer, 0, BUFFER_LENGTH);
        c->status = 2;
    } else if(c->status == 2) {
        c->wlength = 0;
        memset(c->wbuffer, 0, BUFFER_LENGTH);
        c->status = 0;
    }
#elif 0
    // send 图片
    int filefd = open("vip.png", O_RDONLY);
    // 获取文件大小：1.seek(调到最后) 2.fstat
    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    if(c->status == 0) {
        c->wlength = sprintf(c->wbuffer, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type： image/png\r\n"
            "Accept-Ranges: bytes\r\n"
            "Content-Length: %ld\r\n"
            "Date: Thu, 30 Apr 2025 17:44:32 GMT\r\n\r\n",
            stat_buf.st_size
        );
        c->status = 1;
    } else if(c->status == 1){
        int ret = sendfile(c->fd, filefd, NULL, stat_buf.st_size);
        if(ret == -1) {
            printf("errno: %s", strerror(errno));
        }
        // c->wlength = 0;
        // memset(c->wbuffer, 0, BUFFER_LENGTH);
        c->status = 2;
    } else if(c->status == 2) {
        c->wlength = 0;
        memset(c->wbuffer, 0, BUFFER_LENGTH);
        c->status = 0;
    }
#endif
}