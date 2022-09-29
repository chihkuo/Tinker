#!/bin/bash

# set name
TMP_DIR=/run/user/1000
UPDATE_DIR=$TMP_DIR/update
BIN_DIR=/home/linaro/bin

DLS=dlsetting
DLSOCKET=DLsocket.exe
FWUPDATE=FWupdate.exe

sudo killall -9 $DLSOCKET
sudo killall -9 $FWUPDATE
sleep 1
sync

# copy file
cp -f $UPDATE_DIR/$DLS $BIN_DIR/$DLS
chmod 755 $BIN_DIR/$DLS
cp -f $UPDATE_DIR/$DLSOCKET $BIN_DIR/$DLSOCKET
chmod 755 $BIN_DIR/$DLSOCKET
cp -f $UPDATE_DIR/$FWUPDATE $BIN_DIR/$FWUPDATE
chmod 755 $BIN_DIR/$FWUPDATE

sync
sleep 1

$BIN_DIR/$DLSOCKET &
$BIN_DIR/$FWUPDATE &

SWSH=newSWupdate.sh
SWBIN=SWupdate.exe
if [ -f $UPDATE_DIR/$SWBIN ]
then
	cp $UPDATE_DIR/$SWSH $TMP_DIR
	chmod 755 $TMP_DIR/$SWSH
	cp $UPDATE_DIR/$SWBIN $TMP_DIR
	chmod 755 $TMP_DIR/$SWBIN
fi
