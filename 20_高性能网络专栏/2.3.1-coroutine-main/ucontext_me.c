#define _GNU_SOURCE
#include <dlfcn.h>

#include <ucontext.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


ucontext_t ctx[3];
ucontext_t main_ctx;

int count = 0;

//hook
//1)定义类型
typedef ssize_t (*read_t)(int fd, void *buf, size_t count);
read_t read_f = NULL;
typedef ssize_t (*write_t)(int fd, const void *buf, size_t count);
write_t write_f = NULL;
//2)实现原始接口
ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret = read_f(fd, buf, count);
    printf("read: %s\n", (char *)buf);
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
    return write_f(fd, buf, count);
}
//3)hook初始化
void init_hook(void) {
    if(!read_f) {
        read_f = dlsym(RTLD_NEXT, "read");
    }
    if(!write_f) {
        write_f = dlsym(RTLD_NEXT, "read");
    }
}

//coroutine1
void func1(void) {
    while(count ++ < 20) {
        printf("1\n");
        swapcontext(&ctx[0], &main_ctx);
        printf("4\n");
    }
}
//coroutine2
void func2(void) {
    while(count ++ < 20) {
        printf("2\n");
        swapcontext(&ctx[1], &main_ctx);
        printf("5\n");
    }
}
//coroutine3
void func3(void) {
    while(count ++ < 20) {
        printf("3\n");
        swapcontext(&ctx[2], &main_ctx);
        printf("6\n");
    }
}

// 调度器 schedule
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

#if 0
    char stack1[2048] = {0};
    char stack2[2048] = {0};
    char stack3[2048] = {0};

    getcontext(&ctx[0]);
    ctx[0].uc_stack.ss_sp = stack1;
    ctx[0].uc_stack.ss_size = sizeof(stack1);
    ctx[0].uc_link = &main_ctx;
    makecontext(&ctx[0], func1, 0);

    getcontext(&ctx[1]);
    ctx[1].uc_stack.ss_sp = stack2;
    ctx[1].uc_stack.ss_size = sizeof(stack1);
    ctx[1].uc_link = &main_ctx;
    makecontext(&ctx[1], func2, 0);

    getcontext(&ctx[2]);
    ctx[2].uc_stack.ss_sp = stack3;
    ctx[2].uc_stack.ss_size = sizeof(stack3);
    ctx[2].uc_link = &main_ctx;
    makecontext(&ctx[2], func3, 0);
    printf("swapcontext\n");

    while(count <= 30) { //scheduler，调度器策略，切换协程的规则，需要根据实际业务进行更改
        swapcontext(&main_ctx, &ctx[count%3]);
    }
    printf("\n");
#endif

    return 0;
}