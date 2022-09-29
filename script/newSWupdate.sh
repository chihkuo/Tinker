#!/bin/bash

sleep 1

killall -9 SWupdate.exe
sleep 1
sync

cp -f /run/user/1000/SWupdate.exe /home/linaro/bin/
sleep 1
chmod 755 /home/linaro/bin/SWupdate.exe
sync

rm /run/user/1000/newSWupdate.sh
rm /run/user/1000/SWupdate.exe
sync

# reboot after change network setting
sudo reboot