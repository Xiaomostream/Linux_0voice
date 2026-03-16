# 编译方式和运行
```
cd udp-rtt
mkdir build
cd build
cmake ..
make
```

运行时注意服务端监听的端口以及客户要访问的IP+Port

服务端运行
./chat_server 0.0.0.0 10000

客户端运行
./chat_client 192.168.1.28 10000


# 云服务器测试
本地client -> 路由 -> 服务器server -> 路由-> 本地client

## 正常测试
```
rtt[0]: 52ms
rtt[1]: 51ms
rtt[2]: 0ms
rtt[3]: 38ms
rtt[4]: 0ms
rtt[5]: 39ms
rtt[6]: 1ms
rtt[7]: 40ms
rtt[8]: 1ms
rtt[9]: 49ms
rtt[10]: 1ms
rtt[11]: 39ms
rtt[12]: 1ms
rtt[13]: 39ms
rtt[14]: 1ms
rtt[15]: 39ms
rtt[16]: 1ms
rtt[17]: 39ms
rtt[18]: 1ms
rtt[19]: 39ms
rtt avg:23ms, min:0ms, max:52ms
rtt avg:23ms, min:0ms, max:52ms
rtt avg:23ms, min:0ms, max:52ms
rtt avg:23ms, min:0ms, max:52ms
```
## 模拟10%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
```
设置参数：sudo tc qdisc add dev ens33 root netem loss 10%
删除方法：sudo tc qdisc del dev ens33 root netem loss 10%
```
**TCP测试结果（tcp-rtt程序）**：
```
rtt avg:66ms, min:24ms, max:330ms
rtt avg:74ms, min:24ms, max:336ms
rtt avg:44ms, min:25ms, max:365ms
rtt avg:41ms, min:24ms, max:143ms
rtt avg:58ms, min:24ms, max:339ms
```
**当前程序测试结果**（默认配置，可以调整client和server的kcp参数设置）
```
rtt avg:38ms, min:1ms, max:131ms
rtt avg:24ms, min:0ms, max:159ms
rtt avg:29ms, min:0ms, max:130ms
rtt avg:35ms, min:0ms, max:140ms
rtt avg:50ms, min:0ms, max:359ms  // 默认的kcp设置也有延迟比较大的情况，可以考虑client和server都开启低延迟测试
rtt[0]: 51ms
rtt[1]: 39ms
rtt[2]: 2ms
rtt[3]: 38ms
rtt[4]: 120ms
rtt[5]: 1ms
rtt[6]: 29ms
rtt[7]: 20ms
rtt[8]: 10ms
rtt[9]: 31ms
rtt[10]: 129ms
rtt[11]: 1ms
rtt[12]: 39ms
rtt[13]: 10ms
rtt[14]: 30ms
rtt[15]: 131ms
rtt[16]: 1ms
rtt[17]: 48ms
rtt[18]: 1ms
rtt[19]: 38ms
rtt avg:38ms, min:1ms, max:131ms
```
## 模拟30%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
设置参数：sudo tc qdisc add dev ens33 root netem loss 30%
删除方法：sudo tc qdisc del dev ens33 root netem loss 30%
**TCP测试结果（tcp-rtt程序）**：
```
rtt avg:183ms, min:23ms, max:842ms
rtt avg:372ms, min:25ms, max:3663ms
rtt avg:233ms, min:25ms, max:1740ms
```
**当前程序测试结果**（默认配置，可以调整client和server的kcp参数设置）：
```
rtt avg:157ms, min:1ms, max:1530ms
rtt avg:118ms, min:39ms, max:760ms
rtt avg:93ms, min:31ms, max:359ms
rtt avg:142ms, min:30ms, max:770ms
rtt[0]: 39ms
rtt[1]: 151ms
rtt[2]: 360ms
rtt[3]: 160ms
rtt[4]: 160ms
rtt[5]: 39ms
rtt[6]: 151ms
rtt[7]: 39ms
rtt[8]: 31ms
rtt[9]: 160ms
rtt[10]: 50ms
rtt[11]: 360ms
rtt[12]: 40ms
rtt[13]: 159ms
rtt[14]: 30ms
rtt[15]: 30ms
rtt[16]: 30ms
rtt[17]: 50ms
rtt[18]: 40ms
rtt[19]: 770ms
```

## 模拟40%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
设置参数：sudo tc qdisc add dev ens33 root netem loss 40%
删除方法：sudo tc qdisc del dev ens33 root netem loss 40%

 **TCP测试结果（tcp-rtt程序）**：
 ```
rtt avg:255ms, min:24ms, max:1744ms     
rtt avg:1001ms, min:25ms, max:16790ms // 有些比较极端的值
rtt avg:140ms, min:24ms, max:501ms
rtt avg:215ms, min:26ms, max:1954ms
```
**当前程序测试结果（默认配置，可以调整client和server的kcp参数设置**） // 总体而言平均延迟更低
```
rtt avg:168ms, min:40ms, max:760ms
rtt avg:151ms, min:39ms, max:1560ms
rtt avg:124ms, min:39ms, max:366ms
rtt avg:148ms, min:40ms, max:761ms
rtt avg:193ms, min:39ms, max:1560ms
rtt[0]: 160ms
rtt[1]: 40ms
rtt[2]: 40ms
rtt[3]: 39ms
rtt[4]: 41ms
rtt[5]: 40ms
rtt[6]: 160ms
rtt[7]: 1560ms
rtt[8]: 40ms
rtt[9]: 40ms
rtt[10]: 40ms
rtt[11]: 161ms
rtt[12]: 159ms
rtt[13]: 40ms
rtt[14]: 39ms
rtt[15]: 161ms
rtt[16]: 40ms
rtt[17]: 160ms
rtt[18]: 39ms
rtt[19]: 40ms
```
### 在模拟丢包40%基础上开启低延迟
**客户端服务端参数设置为**：
```
opt.nodelay = 1;
opt.resend = 1;
```
**测试结果，明显发包-收包速度更快**
```
rtt avg:50ms, min:0ms, max:250ms
rtt avg:57ms, min:0ms, max:290ms
rtt avg:38ms, min:0ms, max:161ms
rtt avg:29ms, min:0ms, max:100ms
rtt[0]: 80ms
rtt[1]: 39ms
rtt[2]: 9ms
rtt[3]: 21ms
rtt[4]: 60ms
rtt[5]: 0ms
rtt[6]: 129ms
rtt[7]: 1ms
rtt[8]: 69ms
rtt[9]: 60ms
rtt[10]: 60ms
rtt[11]: 0ms
rtt[12]: 29ms
rtt[13]: 1ms
rtt[14]: 59ms
rtt[15]: 1ms
rtt[16]: 30ms
rtt[17]: 0ms
rtt[18]: 70ms
rtt[19]: 0ms
rtt avg:35ms, min:0ms, max:129ms
```
## fixed me

注意到在测试低延迟的时候，有发包到收包时间为0ms的场景，后续需要进一步分析（比如是不是精度问题，用us表示就不会是0值）。



# 参考
- kcp 开源的udp可靠性传输
- kcp-cpp 基于kcp原理实现的cpp版本
