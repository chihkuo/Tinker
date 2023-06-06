#!/bin/bash

function copy() {
	echo "run function copy()"
	if [ ! -d "update" ]; then
		mkdir update
	fi
	cp -f ../datalog-h5000/dlg320.exe ./update/
	cp -f ../DataProgram/DataProgram.exe ./update/
	cp -f ../dlsocket/DLsocket.exe ./update/
	cp -f ../SWupdate/SWupdate.exe ./update/
	cp -f ../FWupdate/FWupdate.exe ./update/
	cp -f ../script/dlsetting ./update/
	cp -f ../script/newSWupdate.sh ./update/
	cp -f ../script/update.sh ./update/
	cp -f ../script/program.sh ./update/
	sync
}

function package() {
	echo "run function package()"
	tar -cvf update_$1.tar update
	sync
}

function clean() {
	echo "run function clean()"
	rm -rf update
	sync
}

function all() {
	echo "run function all()"
	copy;
	package $1;
	clean;
}

if [ $1 == "copy" ]; then
	copy;
fi

if [ $1 == "package" ]; then
	package $2;
fi

if [ $1 == "clean" ]; then
	clean;
fi

if [ $1 == "all" ]; then
	all $2;
fi