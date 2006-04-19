#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/bin/sleep 2

/sbin/kextunload "$LOCPATH/MACJack.kext"

/bin/sleep 2

if [ -e "/System/Library/Extensions/IOXperts80211.kext" ]; then
        /sbin/kextload "/System/Library/Extensions/IOXperts80211.kext"
fi

if [ -e "/System/Library/Extensions/WirelessDriver.kext" ]; then
        /sbin/kextload "/System/Library/Extensions/WirelessDriver.kext"
fi

if [ -e "/System/Library/Extensions/AeroCard.kext" ]; then
        /sbin/kextload "/System/Library/Extensions/AeroCard.kext"
fi

#well see macjack_load
if [ -e "/System/Library/Extensions/CiscoPCCardRadio.kext" ]; then
        /sbin/kextload "/System/Library/Extensions/CiscoPCCardRadio.kext"
fi
