#include "kcp_server.h"
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_set>

#define PORT 10000
#define IP_ADDRESS "0.0.0.0"
// 基于KcpServer封装业务
class ChatServer : public KcpServer {
  public:
    using KcpServer::KcpServer;
    // 接收客户端发送过来的消息
    virtual void HandleMessage(const KcpSession::ptr &session,
                               const std::string &msg) override {
        // TRACE(msg);
        std::stringstream ss;
        ss << "user from [" << session->GetAddrToString() << "] : " << msg;
        Notify(ss.str());
    }
    void HandleConnection(const KcpSession::ptr &session) {
        {   // 注意锁的粒度
            Lock lock(mtx_);
            users_.insert(session);
        }

        std::stringstream ss;
        ss << "user from [" << session->GetAddrToString()
           << "] join the ChatRoom!";
        Notify(ss.str());   // 通知房间的所有人
    }
    void HandleClose(const KcpSession::ptr &session) {
        TRACE("close ", session->GetAddrToString());
        {
            Lock lock(mtx_);
            users_.erase(session);
        }
        std::stringstream ss;
        ss << "user from [" << session->GetAddrToString()
           << "] left the ChatRoom!";
        Notify(ss.str());
    }
    void Notify(const std::string &str) {
        Lock lock(mtx_);

        for (auto &user : users_)
            user->Send(str.data(), str.size());
    }

  public:
    using Lock = std::unique_lock<std::mutex>;

  private:
    std::unordered_set<KcpSession::ptr> users_;
    std::mutex mtx_;
};

int main(int argc, char *argv[]) {
    std::string  ip_addr = IP_ADDRESS;
    int port = PORT;

    // arg : ip + port
    if (argc >= 2) {
         ip_addr = argv[1];
    }
    if (argc >= 3) {
         port = atoi(argv[2]);
    }
    printf("ip:%s, port:%d\n", ip_addr.c_str(), port);  
    KcpOpt opt;
    opt.conv = 0;
    opt.is_server = true;
    opt.keep_alive_timeout = 5000; // 超过5秒没有收到客户端的信息则认为其断开
    // opt.nodelay = 1;   // 对比开启低延迟和不开启低延迟的区别
    // opt.resend = 1;
    ChatServer server(opt, ip_addr, port);

    server.Run();

    return 0;
}