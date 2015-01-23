#!/bin/bash

#IFS_BAK=$IFS
#IFS=$'\n'

output="../success/"
file=1
check=2
num_links=0
the_node=0
dest=0
pos=0
link_ab=0
columns=0
links=0
time=600

while read line
do 
#for line in $(cat success.txt); do
#awk '{print NF}'
#if (echo $line | grep -Fxq "g=")
if [[ $line == *"g="* ]]
then
#	echo "${line}"
	columns=$(echo ${line} | head -n1 |awk '{print NF}')
	num_links=$(( $columns/4))
	the_node=$(echo ${line} | awk '{print $4}')
	echo $num_links
	echo "the_node= $the_node"
	for ((i=0;i<num_links;i++))
	do
		pos=$((8+($num_links*4)))
		dest=$(echo ${line} | awk '{print $$pos}')
		echo $dest
		if [[ $the_node == -1 ]];  then
			link_ab=$(($link_ab+1))
		elif [[ $the_node == $dest ]]; then	
			:
		else
			:
		fi
	done
#	echo "links ${num_links}"
#elif(echo $line | grep -Fxq "t=")
elif [[ $line == *"t="* ]]
then
	#| awk '{print $1}' |
	echo "blocks"
else
	file=${file}+1
	check=2
	links=0
	time=${time}+600
fi
done <success.txt

#IFS=$IFS_BAK
#IFS_BAK=
