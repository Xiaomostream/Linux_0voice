#!/bin/bash
echo "Hello World"

voice="www.xiaomo.com"

echo $voice

for file in $(ls /home/chan/share/); do
	echo "${file}"
done
