#!/bin/sh

AIRPORT_PROG=`dirname "$0"`/AirPortMenu
LOCPATH=`/usr/bin/dirname "$0"`

/bin/echo "Unload called"

/bin/sleep 2

"/sbin/kextunload" "$LOCPATH/WLanDriver.kext"

/bin/sleep 2

#seems to be a little unresponsive :/
if [ -e "/System/Library/Extensions/AppleAirPort.kext" ]; then
	COUNTER=1
	"$AIRPORT_PROG"
	if [ $? -ne 0 ]; then
		/sbin/kextunload -b com.apple.driver.AppleAirPort
		/sbin/kextload -b com.apple.driver.AppleAirPort
		"$AIRPORT_PROG"
	fi
	"$AIRPORT_PROG"
	while [ $? -ne 0 ]; do
		echo "KisMAC: Retrying to load Airport for the $COUNTER time..."
		/bin/sleep $COUNTER
        /sbin/kextunload -b com.apple.driver.AppleAirPort
		/bin/sleep $COUNTER
		/sbin/kextload -b com.apple.driver.AppleAirPort
		/bin/sleep $COUNTER 
		let COUNTER=COUNTER+1
		"$AIRPORT_PROG"
	done;
fi

/bin/sleep 2

#/sbin/ifconfig en1 up
"$AIRPORT_PROG" enable

exit 0