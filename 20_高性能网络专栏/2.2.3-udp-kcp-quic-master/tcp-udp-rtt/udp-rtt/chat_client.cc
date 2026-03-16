
#include "kcp_client.h"
#include "trace.h"
#include <sys/time.h>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex> 
#define PORT 10000
#define IP_ADDRESS "192.168.1.19"
#define MESSAGE_SIZE 1024
#define LOOP_NUM 20
int g_loop_num = LOOP_NUM;
int64_t get_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t milliseconds =
        tv.tv_sec * 1000LL + tv.tv_usec / 1000; // 将秒和微秒转换为毫秒
    return milliseconds;
}

// 基于KcpClient封装业务
class ChatClient : KcpClient {
  public:
    using KcpClient::KcpClient;

    void Start() {
        std::thread t([this]() {
            while (true) {
                usleep(10);     // 客户端

                if (!Run()) {
                    TRACE("error ouccur");
                    break;
                }
            }
        });

        while (true) {
            std::unique_lock<std::mutex> lck(mtx_syn);
            // std::string msg;
            // std::getline(std::cin, msg);        // 读取用户输入数据
            // Send(msg.data(), msg.length());     // 发送消息
            char message[MESSAGE_SIZE];
            memset(message, 'a', MESSAGE_SIZE);
            start_time = get_ms();
            Send(message, MESSAGE_SIZE);     // 发送消息
            cv_syn.wait(lck);      // 等待
        }
        t.join();   // 等待线程退出
    }
    // 服务端发送过来的消息
    virtual void HandleMessage(const std::string &msg) override {
        // std::cout << msg << std::endl;  // 不打印
        // 计算RTT
        int64_t rtt = get_ms() - start_time;
        rtt_vec.push_back(rtt);
        loop_num++;
        if(loop_num >= g_loop_num) {
            loop_num = 0;
            total_rtt = 0;
            min_rtt = rtt_vec[0];
            max_rtt = rtt_vec[0];
            for (int i = 0; i < g_loop_num; i++) {
                printf("rtt[%d]: %ldms\n", i, rtt_vec[i]);
                total_rtt += rtt_vec[i]; // 计算总的rtt，方便后续取平均
                if (rtt_vec[i] < min_rtt)
                    min_rtt = rtt_vec[i];
                if (rtt_vec[i] > max_rtt)
                    max_rtt = rtt_vec[i];
            }
            printf("rtt avg:%ldms, min:%ldms, max:%ldms\n",
                int64_t(total_rtt / g_loop_num), min_rtt, max_rtt);
            rtt_vec.clear();
        }
        std::unique_lock<std::mutex> lck(mtx_syn);
        cv_syn.notify_one();  // 唤醒继续发送消息
    }
    virtual void HandleClose() override {
        std::cout << "close kcp connection!" << std::endl;
        exit(-1);
    }
    std::vector<int64_t> rtt_vec;
    std::mutex mtx_syn;
    std::condition_variable cv_syn;
    int loop_num = 0;
    int64_t start_time = 0;
    int64_t total_rtt = 0;
    int64_t min_rtt = 0;
    int64_t max_rtt = 0;
};

int main(int argc, char *argv[]) {
    std::string  ip_addr = IP_ADDRESS;
    int port = PORT;
    g_loop_num = LOOP_NUM;
    std::vector<int64_t> rtt_vec;

    if (argc >= 2)
        ip_addr = argv[1];
    if (argc >= 3)
        port = atoi(argv[2]);
    if (argc >= 4)
        g_loop_num = atoi(argv[3]);
    
    printf("ip:%s, port:%d, loop_num:%d\n", ip_addr.c_str(), port, g_loop_num);   
 

    srand(time(NULL));

    KcpOpt opt;
    opt.conv = rand() * rand(); // uuid的函数
    opt.is_server = false;
    opt.keep_alive_timeout = 1000; // 保活心跳包间隔，单位ms
    // opt.nodelay = 1;  // 默认不开启低延迟，测试时候可以打开这里的注释做对比
    // opt.resend = 1;
    TRACE("conv = ", opt.conv); 
    ChatClient client(ip_addr, port, opt);

    client.Start();

    return 0;
}