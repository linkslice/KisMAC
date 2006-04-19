#!/bin/sh

ROOT_DIR="$3"

if [ -e "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver.old" ]; then
        /sbin/kextunload "/System/Library/Extensions/WirelessDriver.kext"
        /bin/rm "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver"
        /bin/mv "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver.old" "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver"
        /usr/sbin/kextcache -L -N -k -e
fi

if [ -e "/System/Library/Extensions/WirelessDriver.kext/bipatch" ]; then
        /bin/rm "/System/Library/Extensions/WirelessDriver.kext/bipatch"
fi
