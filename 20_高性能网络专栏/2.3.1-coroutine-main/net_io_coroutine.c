#define _GNU_SOURCE
#include <dlfcn.h>

#include <ucontext.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

struct coroutine {
  int fd;
  ucontext_t ctx;
  void *arg;

  //ready, wait, sleep, (exit)
  queue_node(coroutine, ) ready_queue; //就绪的数据结构用队列来做
  //但是等待的数据结构不适合用队列，因为每个等待的时间不确定，可以使用红黑树（跳表、哈希等查找的数据结构）
  rbtree_node(coroutine, ) wait_rb;
  rbtree_node(coroutine, ) sleep_rb;
  //sleep也是同样道理，使用红黑树 

};
typedef void *(*coroutine_entry)(void *);
int create_coroutine(co_id id, coroutine_entry entry, void *arg) {
  struct coroutine *co = (struct coroutine *)malloc(sizeof(struct coroutine));
  co->ss_sp = ;
  makecontext(co->ctx, func, 0);
}

//调度器
struct scheduler{ 
  int epfd; 
  struct epoll_event events[];
  queue_node(coroutine, ) ready_queue; 
  rbtree_node(coroutine, ) wait_rb;
  rbtree_node(coroutine, ) sleep_rb;
};

context_switch