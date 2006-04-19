#!/bin/sh

ROOT_DIR="$1"

if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/MACJack.kext" ]; then
	/usr/sbin/chown -R root:wheel "$ROOT_DIR/KisMAC.app/Contents/Resources/MACJack.kext"
fi

if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/AiroJack.kext" ]; then
	/usr/sbin/chown -R root:wheel "$ROOT_DIR/KisMAC.app/Contents/Resources/AiroJack.kext"
fi

if [ -e "$ROOT_DIR/KisMAC.app/Contents/Resources/WLanDriver.kext" ]; then
	/usr/sbin/chown -R root:wheel "$ROOT_DIR/KisMAC.app/Contents/Resources/WLanDriver.kext"
fi

/usr/sbin/chown root:admin $ROOT_DIR/KisMAC.app/Contents/Resources/*.sh 
/bin/chmod 755 $ROOT_DIR/KisMAC.app/Contents/Resources/*.sh

/usr/sbin/chown root:admin $ROOT_DIR/KisMAC.app/Contents/Resources/AirPortMenu 
/bin/chmod 755 $ROOT_DIR/KisMAC.app/Contents/Resources/AirPortMenu
