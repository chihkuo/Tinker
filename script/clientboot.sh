#!/bin/bash

touch /home/linaro/clientboot.txt

sleep 10
sudo killall wpa_supplicant
sleep 1
sudo /sbin/wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf
