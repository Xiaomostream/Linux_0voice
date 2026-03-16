#include <stdio.h>
#include <setjmp.h>
jmp_buf env;
void func(int arg) {
    printf("func: %d\n", arg);
    longjmp(env, ++arg);
}

int main() {
    int ret = setjmp(env);
    if(ret == 0) func(ret);
    else if(ret == 1) func(ret);
    else if(ret == 2) func(ret);
    else if(ret == 3) func(ret);
    return 0;
}