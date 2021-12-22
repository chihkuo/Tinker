#!/bin/bash

if [ "$1" = "ap" ]
then
	echo "var is ap"
	echo "copy dhcpcd.conf_ap"
	sudo cp -f /home/linaro/init/configs/dhcpcd.conf_ap2 /etc/dhcpcd.conf
	echo "copy hostapd_ap"
	sudo cp -f /home/linaro/init/configs/hostapd_ap /etc/default/hostapd
	echo "copy dnsmasq.conf_ap"
	sudo cp -f /home/linaro/init/configs/dnsmasq.conf_ap /etc/dnsmasq.conf
#	echo "copy interfaces_ap"
#	sudo cp -f /home/linaro/init/configs/interfaces_ap /etc/network/interfaces
	echo "copy wpa_supplicant.conf_default"
	sudo cp -f /home/linaro/init/configs/wpa_supplicant.conf_default /etc/wpa_supplicant/wpa_supplicant.conf
	echo "copy apboot.sh"
	cp -f /home/linaro/init/apboot.sh /home/linaro/init/myinit.sh
	sync
fi

if [ "$1" = "client" ]
then
	echo "var is client"
	echo "copy dhcpcd.conf_default"
	sudo cp -f /home/linaro/init/configs/dhcpcd.conf_default /etc/dhcpcd.conf
	echo "copy hostapd_default"
	sudo cp -f /home/linaro/init/configs/hostapd_default /etc/default/hostapd
	echo "copy dnsmasq.conf_default"
	sudo cp -f /home/linaro/init/configs/dnsmasq.conf_default /etc/dnsmasq.conf
#	echo "copy interfaces_default"
#	sudo cp -f /home/linaro/init/configs/interfaces_default /etc/network/interfaces
	echo "copy wpa_supplicant.conf_set"
	sudo cp -f /home/linaro/init/configs/wpa_supplicant.conf_set /etc/wpa_supplicant/wpa_supplicant.conf
	echo "copy clientboot.sh"
	cp -f /home/linaro/init/clientboot.sh /home/linaro/init/myinit.sh
	sync
fi
