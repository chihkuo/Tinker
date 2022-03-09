#!/bin/bash

if [ "$1" = "ap" ]
then
	# wait network OK
	sleep 10

	ps -ax | grep DLsocket.exe | grep -v grep
	if [ $? != 0 ]
	then
		echo "run DLsocket.exe"
		/home/linaro/bin/DLsocket.exe &
	else
		echo "DLsocket.exe alive"
	fi

	sleep 1

	ps -ax | grep dlg320.exe | grep -v grep
	if [ $? != 0 ]
	then
		echo "run dlg320.exe"
		/home/linaro/bin/dlg320.exe &
	else
		echo "dlg320.exe alive"
	fi

	sleep 60

	ps -ax | grep DataProgram.exe | grep -v grep
	if [ $? != 0 ]
	then
		echo "run DataProgram.exe"
		/home/linaro/bin/DataProgram.exe &
	else
		echo "DataProgram.exe alive"
	fi

fi

if [ "$1" = "client" ]
then

	ps -ax | grep dlg320.exe | grep -v grep
	if [ $? != 0 ]
	then
		echo "run dlg320.exe"
		/home/linaro/bin/dlg320.exe &
	else
		echo "dlg320.exe alive"
	fi

	sleep 60

	ps -ax | grep DataProgram.exe | grep -v grep
	if [ $? != 0 ]
	then
		echo "run DataProgram.exe"
		/home/linaro/bin/DataProgram.exe &
	else
		echo "DataProgram.exe alive"
	fi
fi

if [ "$1" = "stop" ]
then
	echo "stop"
	sudo killall -9 DataProgram.exe dlg320.exe DLsocket.exe
fi