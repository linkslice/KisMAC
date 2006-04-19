#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/bin/sleep 2

"/sbin/kextunload" "$LOCPATH/AiroJack.kext"

/bin/sleep 2

if [ -e "/System/Library/Extensions/CiscoPCCardRadio.kext" ]; then
        /sbin/kextload "/System/Library/Extensions/CiscoPCCardRadio.kext"
fi

