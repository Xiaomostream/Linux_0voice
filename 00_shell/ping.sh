#!/bin/bash
for i in {1..254}; do
    # ping所有局域网， &>/dev/null
    ping -c 2 -i 0.5 192.168.66.$i &>/dev/null
    # $? 表示上条命令运行成功与否
    if [ $? -eq 0 ]; then
        echo "192.168.66.$i is up"
    else
        echo "192.168.66.$i is down"
    fi
done 
