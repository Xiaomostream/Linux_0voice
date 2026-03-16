#define _GNU_SOURCE
#include <dlfcn.h>

#include <ucontext.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main() {
    init_hook();

    int fd = open("a.txt", O_CREAT|O_RDWR);
    if(fd < 0) {
        return -1;
    }
    char *str = "1234567890";
    write(fd, str, strlen(str));
    
    char buffer[128] = {0};
    read(fd, buffer, 128);
    printf("buffer: %s\n", buffer);
}