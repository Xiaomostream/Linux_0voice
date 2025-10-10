/*
一. 线程池使用场景
    1. 避免线程太多，使得内存耗尽
        posix下，一个线程需要8M的运行内存, 16G内存的服务器最多只能开 16*128=2048 个线程
    2. 避免创建与销毁线程的代价  
    3. 任务与执行分离
        eg1. 日志文件，磁盘操作往往比内存操作慢汗多，写线程的时候，会引起线程的挂起
            写日志的任务 | 执行任务 通过线程池做到分离
        eg2. 营业厅
            办业务的人(任务队列) | 柜员(执行队列) | 公示牌(管理组件) -> 锁
            营业厅里面的公式牌:
                防止多个办业务的人，在一个柜员里面办业务;放在两个柜员同时为一个人办业务，使得营业厅能够正常有序的工作
    4. 组成：任务队列 执行队列 管理组件
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

int mp[1001], count;

#define LIST_INSERT(item, list) do{       \
    item->next = list;                    \
    item->prev = NULL;                    \
    if(list != NULL) (list)->prev = item; \
    list = item;                          \
} while(0)

#define LIST_REMOVE(item, list) do{                         \
    if(item->prev != NULL) item->prev->next = item->next;   \
    if(item->next != NULL) item->next->prev = item->prev;   \
    if(list == item) list = item->next;                     \
    item->prev = item->next = NULL;                         \
} while(0)

// 线程池的结构与定义

// 任务队列
struct nTask{
    void (*task_func)(struct nTask *task); //用来存储执行的任务
    void *user_data; //用来执行任务的参数

    struct nTask *prev;
    struct nTask *next;
};

// 执行队列
struct nWorker{
    pthread_t threadid;
    int terminate; //是否终止
    struct nManager *manager;

    struct nWorker *prev;
    struct nWorker *next;
};

// 管理组件
typedef struct nManager{
    struct nTask *tasks;
    struct nWorker *workers;

    pthread_mutex_t mutex; //控制有序执行
    pthread_cond_t cond; //条件变量，判断是否还需等待
} ThreadPool;

// API
// 线程池回调函数 (callback != task)
static void *nThreadPoolCallback(void *arg) {
    //printf("nThreadPoolCallback\n");
    // 线程就是判断任务队列里是否有任务，一旦有任务，就从任务队列中取出一个任务，没有任务，则一直等待新任务的到来
    struct nWorker *worker = (struct nWorker*)arg;
    while(1) {
        // 进入阻塞，等待条件，直到新任务到来
        pthread_mutex_lock(&worker->manager->mutex);
        while(worker->manager->tasks == NULL) {
            if(worker->terminate) break;
            pthread_cond_wait(&worker->manager->cond, &worker->manager->mutex);
        }
        if(worker->terminate) {
            //这里记得解锁，不然会发生死锁
            pthread_mutex_unlock(&worker->manager->mutex);
            break;
        }

        //有任务了，把该任务队列的首节点拿出来
        struct nTask *task = worker->manager->tasks;
        LIST_REMOVE(task, worker->manager->tasks);

        pthread_mutex_unlock(&worker->manager->mutex);

        //执行该任务
        task->task_func(task);
    }

    //一旦退出就free掉
    free(worker);
}

// 创建线程池
int nThreadPoolCreate(ThreadPool *pool, int numWorkers) {
    //printf("nThreadPoolCreate\n");
    // 初始化参数与每一个域
    if(pool == NULL) return -1;
    if(numWorkers < 1) numWorkers = 1;
    memset(pool, 0, sizeof(ThreadPool));

    // 初始化条件变量
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool->cond, &blank_cond, sizeof(pthread_cond_t));

    // 初始化锁
    // pthread_mutex_init(&pool->mutex, NULL);
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->mutex, &blank_mutex, sizeof(pthread_mutex_t));

    // 初始化numWorkers个线程(执行队列)
    int i = 0;
    for (i = 0; i < numWorkers; i ++) {
        struct nWorker *worker = (struct nWorker*)malloc(sizeof(struct nWorker));
        if(worker == NULL) {
            perror("mallc");
            return -2;
        }
        // 堆上的数据记得初始化
        memset(worker, 0, sizeof(struct nWorker));
        
        worker->manager = pool;
        int ret = pthread_create(&worker->threadid, NULL, nThreadPoolCallback, worker);
        if(ret) { //创建成功返回0，创建失败返回非0
            perror("pthread_create");
            free(worker);
            return -3;
        }
        // 把worker插入线程池的workers
        LIST_INSERT(worker, pool->workers);
    }
    return 0;
}

// 销毁线程池
int nThreadPoolDestory(ThreadPool *pool, int nWorker) {
    //printf("nThreadPoolDestory\n");
    struct nWorker *worker = NULL;
    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }

    //这里做一个条件广播，使得pthread_cond_wait成立
    //这里和在条件等待时用的是同一把锁，避免了死锁的发生
    pthread_mutex_lock(&pool->mutex);
    pthread_cond_broadcast(&pool->cond); //唤醒所有等待这个条件的线程
    pthread_mutex_unlock(&pool->mutex);

    pool->workers = NULL;
    pool->tasks = NULL;

    return 0;
}

// 往线程池中添加任务
int nThreadPoolPushTask(ThreadPool *pool, struct nTask *task) {
    //printf("nThreadPoolPushTask\n");
    pthread_mutex_lock(&pool->mutex);
    LIST_INSERT(task, pool->tasks);
    pthread_cond_signal(&pool->cond); //唤醒一个等待这个条件的线程
    pthread_mutex_unlock(&pool->mutex);
}

// SDK --> Debug ThreadPool

#if 1

#define THREADPOOL_INIT_COUNT      20
#define TASK_INIT_SIZE             1000

void task_entry(struct nTask *task){
    //struct nTask *task = (struct nTask*)task;
    int idx = *(int *)task->user_data;
    printf("idx: %d\n", idx);
    if(mp[idx] == 0) {
        count ++;
        mp[idx] = 1;
    }
    free(task->user_data);
    free(task);
}

int main(void) {
    ThreadPool pool = {0};
    //这里pool记得在Create里初始化为空
    nThreadPoolCreate(&pool, THREADPOOL_INIT_COUNT);
    printf("nThreadPoolCreate -- finished\n");

    int i = 0;
    for (i = 0; i < TASK_INIT_SIZE; i ++) {
        struct nTask *task = (struct nTask *)malloc(sizeof(struct nTask));
        if(task == NULL) {
            perror("malloc");
            exit(1);
        }
        memset(task, 0, sizeof(struct nTask));

        task->task_func = task_entry; //在子函数中释放task
        task->user_data = malloc(sizeof(int));
        *(int*)task->user_data = i;

        nThreadPoolPushTask(&pool, task);
    }
    getchar(); //如果不加getchar，会由于主线程结束而导致程序提前结束
    printf("count = %d\n", count);
}   

#endif