#!/bin/bash

touch /home/linaro/clientboot.txt

sleep 5
sudo killall wpa_supplicant
sleep 1
sudo /sbin/wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf

if [[ -e '/home/linaro/apboot.txt' ]]; then
	sleep 10
	echo "check network"
	sudo ping www.google.com -w 5
	if [[ $? == 0 ]]; then
		echo "network ok, remove apboot.txt"
		rm /home/linaro/apboot.txt
	else
		echo "network fail, return AP mode"
		/home/linaro/init/setmode.sh ap
		sync
		sudo reboot
	fi
fi
