# 编译方式和运行
```
cd udp-kcp
mkdir build
cd build
cmake ..
make
```

运行时注意服务端监听的端口以及客户要访问的IP+Port

服务端运行
./chat_server 0.0.0.0 10001

客户端运行
./chat_client 192.168.1.28 10001



# 参考
- kcp 开源的udp可靠性传输
- kcp-cpp 基于kcp原理实现的cpp版本