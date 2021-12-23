#!/bin/bash

echo "kill wpa_supplicant"
sudo killall wpa_supplicant
sleep 0.5
echo "connect wifi ap"
sudo /sbin/wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf

if [[ -e '/home/linaro/apboot.txt' ]]; then
	echo "check network"
	for var in $(seq 1 6)
	do
		sleep 10
		sudo ping www.google.com -w 5
		if [[ $? == 0 ]]; then
			echo "network ok, remove apboot.txt, make clientboot.txt"
			rm /home/linaro/apboot.txt
			touch /home/linaro/clientboot.txt
			exit 0
		else
			echo "var = $var"
			if [[ $var = 6 ]]; then
				echo "network fail, return AP mode"
				/home/linaro/init/setmode.sh ap
				sync
				sudo reboot
			fi
		fi
	done
else
	touch /home/linaro/clientboot.txt
fi
