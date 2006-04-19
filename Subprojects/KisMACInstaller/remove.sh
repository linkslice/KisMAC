#!/bin/sh

ROOT_DIR="$1"

#may be some is that stupid
/usr/bin/killall "KisMAC"

if [ -e "$ROOT_DIR/KisMAC.app" ]; then
	killall KisMAC
	if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/MacJack.kext" ]; then
		kextunload "$ROOT_DIR/KisMAC.app/Contents/Resources/MacJack.kext"
	fi
	if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/AiroJack.kext" ]; then
		kextunload "$ROOT_DIR/KisMAC.app/Contents/Resources/AiroJack.kext"
	fi
	if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/WLanDriver.kext" ]; then
		kextunload "$ROOT_DIR/KisMAC.app/Contents/Resources/WLanDriver.kext"
	fi
	/bin/rm -rf "$ROOT_DIR/KisMAC.app"
fi

if [ -e "$ROOT_DIR/KisMAC Driver Tool.app" ]; then
        /bin/rm -rf "$ROOT_DIR/KisMAC Driver Tool.app"
fi

if [ -e "$ROOT_DIR/WirelessMAC.app" ]; then
        /bin/rm -rf "$ROOT_DIR/WirelessMAC.app"
fi

for USERS in `/bin/ls /Users`; do
        if [ -e "/Users/$USERS/Library/Logs/CrashReporter/KisMAC.crash.log" ]; then
		/bin/rm -f "/Users/$USERS/Library/Logs/CrashReporter/KisMAC.crash.log"
	fi
done	

/usr/bin/touch "$ROOT_DIR"
