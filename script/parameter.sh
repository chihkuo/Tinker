#!/bin/sh

# all item & default value
#sms_server=http://portal.darfon.com
#sms_port=8080
#update_server=http://portal.darfon.com
#update_port=8080
#sample_time=300
#upload_time=300
#update_SW_time=5
#delay_time_1=20000000
#delay_time_2=1500000
#cleartx_delay=200000
#shelf_life=30
#reboot_time=1
#update_FW_start=0
#update_FW_stop=0
#com1_baud=9600
#com1_data_bits=8
#com1_parity=None
#com1_stop_bits=1
#dlver=2.8.2
#dpver=2.8.2
#fuver=1.2.9
#dsuver=2.4.5
#dsver=1.0.0

# get data command
#./parameter.sh get item
# set data command
#./parameter.sh set item value
# if set sms_server or update_server, value format example : "http:\/\/portal.darfon.com", i.e.
#./parameter.sh set sms_server "http:\/\/portal.darfon.com"

DEBUG=N
if [ $DEBUG = "Y" ]
then
	echo "num = $#"
	echo "type = $1"
	echo "item = $2"
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
