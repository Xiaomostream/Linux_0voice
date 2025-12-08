#include <sys/socket.h> 
#include <stdio.h>
int main()
{   
    fd_set fdset;   
    FD_ZERO(&fdset);   
    FD_SET(1, &fdset);   
    FD_SET(2, &fdset);   
    FD_SET(3, &fdset);   
    FD_SET(7, &fdset);   
    int isset = FD_ISSET(3, &fdset);   
    printf("isset = %d\n", isset);   
    FD_CLR(3, &fdset);   
    isset = FD_ISSET(3, &fdset);   
    printf("isset = %d\n", isset);   
    return 0;
}