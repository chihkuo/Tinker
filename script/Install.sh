#!/bin/bash

#about boot
#copy Tinker_boot to /etc/init.d
sudo cp Tinker_boot /etc/init.d
#run sudo update-rc.d Tinker_boot defaults
sudo update-rc.d Tinker_boot defaults
#sudo mv /etc/rc2345.d/S03Tinker_boot /etc/rc2345.d/S90Tinker_boot (set order)
sudo mv /etc/rc2.d/S03Tinker_boot /etc/rc2.d/S90Tinker_boot
sudo mv /etc/rc3.d/S03Tinker_boot /etc/rc3.d/S90Tinker_boot
sudo mv /etc/rc4.d/S03Tinker_boot /etc/rc4.d/S90Tinker_boot
sudo mv /etc/rc5.d/S03Tinker_boot /etc/rc5.d/S90Tinker_boot
#Tinker_boot run /home/linaro/init/myinit.sh, see about config

#about etc config
#copy dnsmasq.conf to /etc/
sudo cp configs/dnsmasq.conf /etc/
#copy hostapd to /etc/default/
sudo cp configs/hostapd /etc/default/
#copy hostapd.conf to /etc/hostapd/
sudo cp configs/hostapd.conf /etc/hostapd/

#about config
mkdir /home/linaro/init
#copy apboot.sh clientboot.sh myinit.sh setmode.sh program.sh to init
cp apboot.sh clientboot.sh myinit.sh setmode.sh program.sh /home/linaro/init/
#copy dir. configs to init
cp -r configs /home/linaro/init/

#about bin
mkdir /home/linaro/bin
#copy dlsetting ModelList parameter.sh G320.ini to bin
cp dlsetting ModelList parameter.sh G320.ini /home/linaro/bin/
#copy all excutable files to bin
cp ../datalog-h5000/dlg320.exe /home/linaro/bin/
cp ../DataProgram/DataProgram.exe /home/linaro/bin/
cp ../dlsocket/DLsocket.exe /home/linaro/bin/
cp ../SWupdate/SWupdate.exe /home/linaro/bin/

#about GPIO lib for C
#git clone http://github.com/TinkerBoard/gpio_lib_c.git
#cd gpio_lib_c/
#sudo ./build
#check
#gpio -v
#gpio readall

#about dnsmasq
sudo apt-get install dnsmasq
