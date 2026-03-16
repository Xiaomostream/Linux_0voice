# tcp rtt模拟测试
编译方式：
g++ -o rtt_client rtt_client.cpp
gcc -o rtt_server rtt_server.c

# 本机测试
本机client -> 局域网路由 -> 本机server -> 局域网路由->本机client



这个没啥好比的，重点测试本地 - 云服务器。

```
rtt[0]: 1ms
rtt[1]: 0ms
rtt[2]: 0ms
rtt[3]: 0ms
rtt[4]: 0ms
rtt[5]: 1ms
rtt[6]: 0ms
rtt[7]: 0ms
rtt[8]: 0ms
rtt[9]: 0ms
rtt avg:0ms, min:0ms, max:1ms
```

# 云服务器测试
本地client -> 路由 -> 服务器server -> 路由-> 本地client

## 正常测试
```
rtt[0]: 31ms
rtt[1]: 29ms
rtt[2]: 25ms
rtt[3]: 32ms
rtt[4]: 30ms
rtt[5]: 26ms
rtt[6]: 33ms
rtt[7]: 31ms
rtt[8]: 30ms
rtt[9]: 28ms
rtt avg:29ms, min:25ms, max:33ms
```
## 模拟10%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
设置参数：sudo tc qdisc add dev ens33 root netem loss 10%
删除方法：sudo tc qdisc del dev ens33 root netem loss 10%
```
rtt[0]: 29ms
rtt[1]: 27ms
rtt[2]: 330ms
rtt[3]: 29ms
rtt[4]: 27ms
rtt[5]: 117ms
rtt[6]: 30ms
rtt[7]: 27ms
rtt[8]: 27ms
rtt[9]: 329ms
rtt[10]: 29ms
rtt[11]: 109ms
rtt[12]: 30ms
rtt[13]: 32ms
rtt[14]: 30ms
rtt[15]: 27ms
rtt[16]: 28ms
rtt[17]: 26ms
rtt[18]: 28ms
rtt[19]: 24ms
rtt avg:66ms, min:24ms, max:330ms
rtt avg:74ms, min:24ms, max:336ms
rtt avg:44ms, min:25ms, max:365ms
rtt avg:41ms, min:24ms, max:143ms
rtt avg:58ms, min:24ms, max:339ms
```
## 模拟30%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
```
设置参数：sudo tc qdisc add dev ens33 root netem loss 30%
删除方法：sudo tc qdisc del dev ens33 root netem loss 30%
```
```
rtt[0]: 27ms
rtt[1]: 842ms
rtt[2]: 29ms
rtt[3]: 31ms
rtt[4]: 31ms
rtt[5]: 26ms
rtt[6]: 131ms
rtt[7]: 32ms
rtt[8]: 26ms
rtt[9]: 338ms
rtt[10]: 23ms
rtt[11]: 336ms
rtt[12]: 29ms
rtt[13]: 823ms
rtt[14]: 25ms
rtt[15]: 29ms
rtt[16]: 31ms
rtt[17]: 29ms
rtt[18]: 808ms
rtt[19]: 25ms
rtt avg:183ms, min:23ms, max:842ms
rtt avg:372ms, min:25ms, max:3663ms
rtt avg:233ms, min:25ms, max:1740ms
```
## 模拟40%丢包
模拟丢包，在本地机器设置，注意自己的网卡，我这里本地是ens33
```
设置参数：sudo tc qdisc add dev ens33 root netem loss 40%
删除方法：sudo tc qdisc del dev ens33 root netem loss 40%
```
```
rtt[0]: 263ms
rtt[1]: 320ms
rtt[2]: 342ms
rtt[3]: 25ms
rtt[4]: 28ms
rtt[5]: 30ms
rtt[6]: 27ms
rtt[7]: 802ms
rtt[8]: 500ms
rtt[9]: 31ms
rtt[10]: 30ms
rtt[11]: 30ms
rtt[12]: 343ms
rtt[13]: 16790ms
rtt[14]: 26ms
rtt[15]: 333ms
rtt[16]: 27ms
rtt[17]: 28ms
rtt[18]: 28ms
rtt[19]: 30ms
rtt[0]: 262ms
rtt[1]: 29ms
rtt[2]: 293ms
rtt[3]: 31ms
rtt[4]: 29ms
rtt[5]: 29ms
rtt[6]: 319ms
rtt[7]: 1744ms
rtt[8]: 31ms
rtt[9]: 31ms
rtt[10]: 28ms
rtt[11]: 800ms
rtt[12]: 26ms
rtt[13]: 25ms
rtt[14]: 28ms
rtt[15]: 488ms
rtt[16]: 831ms
rtt[17]: 28ms
rtt[18]: 26ms
rtt[19]: 24ms
rtt avg:255ms, min:24ms, max:1744ms     
rtt avg:1001ms, min:25ms, max:16790ms // 有些比较极端的值
rtt avg:140ms, min:24ms, max:501ms
rtt avg:215ms, min:26ms, max:1954ms
```

