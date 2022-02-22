#!/bin/bash

if [ "$1" = "ap" ]
then
	echo "var is ap"
	echo "copy hostapd.conf"
	sudo cp -f /home/linaro/init/config/hostapd.conf /etc/hostapd/hostapd.conf
	echo "copy apboot.sh"
	cp -f /home/linaro/init/apboot.sh /home/linaro/init/myinit.sh
	sync
fi

if [ "$1" = "client" ]
then
	echo "var is client"
	echo "copy wpa_supplicant.conf"
	sudo cp -f /home/linaro/init/config/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf
	echo "copy clientboot.sh"
	cp -f /home/linaro/init/clientboot.sh /home/linaro/init/myinit.sh
	sync
fi
