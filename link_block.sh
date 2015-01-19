#!/bin/bash

#IFS_BAK=$IFS
#IFS=$'\n'

output="../success/"
file=1
check=2
num_links=0
links=0
time=600

while read line; do
#for line in $(cat success.txt); do
if (echo $line | grep -Fxq "g=")
then
	echo "${line}"
	echo ${line} | head -n1 |awk '{print NF}'
	echo "links ${num_links}"
elif(echo $line | grep -Fxq "t=")
then
	#| awk '{print $1}' |
	echo "blocks"
else
	file=${file}+1
	check=2
	links=0
	time=${time}+600
fi
done

#IFS=$IFS_BAK
#IFS_BAK=
