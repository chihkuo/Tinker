#export STAGING_DIR = /home/chih/toolchain/lede-sdk/staging_dir
#CC = /home/chih/toolchain/lede-sdk/staging_dir/toolchain-mipsel_24kc_gcc-5.4.0_musl-1.1.16/bin/mipsel-openwrt-linux-musl-gcc
LIBDIR = ../datalog-h5000

#export STAGING_DIR = /home/chih/toolchain/openwrt-sdk
#CC = /home/chih/toolchain/openwrt-sdk/staging_dir/toolchain-mipsel_24kc_gcc-7.3.0_musl/bin/mipsel-openwrt-linux-musl-gcc

CC = gcc
CPP = g++
AR = ar

.PHONY: all clean

all:
	$(CC) -Wall main.c ../common/libcommon.a $(LIBDIR)/modbusdrv.o $(LIBDIR)/Darfonlogger.o $(LIBDIR)/iniparser-2.17/src/dictionary.o $(LIBDIR)/iniparser-2.17/src/strlib.o $(LIBDIR)/iniparser-2.17/src/iniparser.o -lpthread -lwiringPi -o FWupdate.exe

clean:
	rm FWupdate.exe
	
