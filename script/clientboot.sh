#!/bin/bash
# client boot

sudo systemctl stop hostapd.service
sudo systemctl stop dnsmqsq.service

sudo ifconfig wlan0 0.0.0.0

sudo nmcli radio wifi on

sudo cp -f /home/linaro/init/config/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf

echo "kill wpa_supplicant"
sudo killall wpa_supplicant
sleep 0.5
echo "connect wifi ap"
sudo /sbin/wpa_supplicant -D nl80211 -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
sleep 5
echo "run dhclient"
sudo dhclient

if [[ -e '/home/linaro/apboot.txt' ]]; then
	echo "check network"
	for var in $(seq 1 6)
	do
		sleep 5
		sudo ping www.google.com -w 5
		if [[ $? == 0 ]]; then
			echo "network ok, remove apboot.txt, make clientboot.txt"
			sudo rm -f /home/linaro/apboot.txt
			sudo su - -c "date > /home/linaro/clientboot.txt"
			exit 0
		else
			echo "var = $var"
			if [[ $var = 6 ]]; then
				echo "network fail, return AP mode"
				/home/linaro/init/setmode.sh ap
				sync
				/home/linaro/init/myinit.sh
			fi
		fi
	done
else
	sudo su - -c "date > /home/linaro/clientboot.txt"
fi