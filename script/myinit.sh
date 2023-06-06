#!/bin/bash
# ap boot

sudo rm -f /home/linaro/clientboot.txt
sudo su - -c "date > /home/linaro/apboot.txt"

sudo cp -f /home/linaro/init/configs/hostapd.conf /etc/hostapd

echo "kill wpa_supplicant"
sudo killall wpa_supplicant

sudo nmcli radio wifi off
sudo ifconfig wlan0 192.168.100.1/24 up

sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
sudo sysctl net.ipv4.ip_forward=1
sudo update-alternatives --set iptables /usr/sbin/iptables-legacy
# for ipv6
sudo update-alternatives --set ip6tables /usr/sbin/ip6tables-legacy

sudo sysctl fs.inotify.max_user_watches=1048576
sudo systemctl daemon-reload
sudo service systemd-resolved stop
sudo systemctl restart dnsmasq.service

sudo systemctl stop hostapd.service
sleep 1
sudo rfkill unblock wifi
sudo systemctl start hostapd.service

echo "run stop"
sudo /home/linaro/init/program.sh stop
echo "run ap"
sudo /home/linaro/init/program.sh ap &