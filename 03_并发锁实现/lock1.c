/*
通过10个线程池模拟火车站抢票问题

一. 用程序模拟这个过程， 10个线程池共享 count 这个资源， 回调函数对count进行++操作
设置回调函数对当前count++十万次，看最后是否是100万次

gcc -o lock lock.c -lpthread

最终由于不同线程抢占，导致最终 count 实际是 < 100万

二. 原因分析
count是一个临界资源(多个线程共享的一个资源)
count ++; 这一行代码其实对应三个指令
1) mov [count], eax;  //把count的值传给寄存器eax
2) inc eax;           //eax+1
3) mov eax, count;    //把寄存器eax的值赋值给count

(1)理想情况下，执行这三条指令时不会进行线程间的切换
(2)但是非正常情况下， 假设当前count = 50, 线程1执行完 1) mov [count], eax 指令后，发生线程的切换，
如果线程2 执行完 这三条指令后count = 51， 此时又切换为线程1执行后续的两条指令，由于寄存器eax，eax的值为50，
count 最后等于51，这就是count < 100万的原因

三. 解决方法

对临界资源count加锁或者使用原子操作

1) 互斥锁 mutex: 把这三条指令锁到一起执行，其他线程进不来，这个过程引起了线程的切换，但是进不来只能等待下一次被调用
使用场景：锁的内容比较多。比如，线程安全的rbtree, 添加可以用mutex 
        pthread_mutex_lock(&mutex);
        (*pcount) ++;
        pthread_mutex_unlock(&mutex);
2) 自旋锁 spinlock: 一旦锁上，相当于使用一个 while(1)，等待该把锁被释放，不会进行线程切换
适用场景：锁的内容很少
具体使用哪把锁的核心是线程切换的代价(mutex)与线程等待的代价(spinlock)

3) 原子操作：把多条指令通过单条CPU指令实现(汇编指令实现):xaddl 1, [count]
由于使用单条CPU指令，根本不可能被分割指向。
    __asm__ volatile(
        "lock; xaddl %2, %1;" //把 %2(add) 加给 %1(*value)
        : "=a" (old)    //返回值给a
        : "m" (*value), "a"(add)
        : "cc", "memory"
    );
4) CAS： Compare And Swap 
CAS 是一种原子操作，核心思想是 "先比较，再交换"，用于解决多线程对共享资源的并发修改问题。
通过比较内存中的值与期望值是否一致，如果一致则交换值。如果不一致，则操作失败并返回当前的值。

CAS操作接收三个参数：
1. 内存地址（V）：要进行比较和交换操作的目标内存位置（例如某个变量的地址）。
2. 旧值（A）：期望的当前值。CAS操作会检查内存中该位置的值是否与此旧值相等。
3. 新值（B）：如果内存中当前值与旧值一致，则将内存中的值更新为新值。

CAS的执行流程：
    - 比较内存位置V的当前值是否等于期望值A。
    - 如果相等，则将V的值替换为新值B。
    - 如果不相等，则操作失败，V的值保持不变，返回当前值。

优点：
    性能优越：CAS避免了传统锁的性能开销，特别是在低竞争环境下。
    无阻塞：相比传统的锁机制，CAS能够避免线程阻塞，减少上下文切换的开销。
    适用性广：CAS是实现各种原子操作的基础，适用于许多并发算法。
缺点：
    高竞争时的性能问题：在高竞争环境下，CAS操作可能会频繁失败，导致大量重试，浪费CPU时间，称为自旋瓶颈。
    ABA问题：如果一个值从A变为B再变回A，CAS操作无法检测到这种变化，导致潜在的错误。这通常可以通过版本号或标记位的技术解决。

*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 10

pthread_mutex_t mutex; //定义一把互斥锁

pthread_spinlock_t spinlock; //定义一把自旋锁

int inc(int *value, int add) {
    //对value这个地址所指向的元素加上 add
    int old;
    __asm__ volatile(
        "lock; xaddl %2, %1;"
        : "=a" (old)
        : "m" (*value), "a"(add)
        : "cc", "memory"
    );
    return old;
}

int CAS(int *value, int expected, int new_val) {
    unsigned char ret;
    __asm__ volatile(
        "lock; cmpxchgl %2, %1;"  // lock前缀保证多核下的原子性，cmpxchgl比较并交换
        "sete %0;"                // 若相等，result=1；否则result=0（sete：相等则置位）
        : "=q"(ret), "+m"(*value)  // 输出：result是结果, *value是读写的内存地址
        : "r"(new_val), "a"(expected) // 输入：new_val是要写的新值，expected在EAX寄存器里
        : "cc", "memory"  // 通知编译器该操作影响条件码寄存器
    );
    return ret;
}
void CAS_inc(int *value) {
    int old, new_val;
    do {
        old = *value;
        new_val = old+1;
    } while(!CAS(value, old, new_val)); // 若CAS失败（值被修改），重试直到成功
}
void *thread_callback(void *arg) {
    int *pcount = (int *)arg;
    // 取出线程创建时传入的 count
    int i = 0;
    while(i ++ < 100000) {
#if 0
        (*pcount) ++;
#elif 0
        pthread_mutex_lock(&mutex);
        (*pcount) ++;
        pthread_mutex_unlock(&mutex);
#elif 0
        pthread_spin_lock(&spinlock);
        (*pcount) ++;
        pthread_spin_unlock(&spinlock);
#elif 0
        inc(pcount, 1);
#else
        CAS_inc(pcount);
#endif
        usleep(1);
    }
}

int main() {
    pthread_t threadid[THREAD_COUNT] = {0};


    pthread_mutex_init(&mutex, NULL);

    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);

    int i = 0;
    int count = 0;  // 共享资源：总票数（初始0，最终应变为100万）
    for (i = 0; i < THREAD_COUNT; i ++) {
        //创建线程函数
        pthread_create(&threadid[i], NULL, thread_callback, &count);
        /*第一个参数 是这个线程返回的的id地址
          第二个参数 是线程的属性，比如堆、栈
          第三个参数 是线程的入口函数
          第四个参数 是往线程的入口函数传入的参数
        */
    }
    for (i = 0; i < 100; i ++) {
        printf("count : %d\n", count);
        sleep(1);
    }
    return 0;
}

