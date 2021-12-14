#!/bin/sh

DEBUG=N
if [ $DEBUG = "Y" ]
then
	echo "num = $#"
	echo "Type = $1"
	echo "Item = $2"
	if [ $# -gt 2 ]
	then
		echo "value = $3"
	fi
fi

if [ "$1" = "get" ]
then
	# get data
	grep $2 dlsetting | awk -F= '{print $2}'
elif [ "$1" = "set" ]
then
	# set data
	sed -i "s/$2=.*/$2=$3/g" dlsetting
	echo "OK"
else
	echo "unknow command"
fi
