#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/usr/bin/killall -KILL "SigMeter" 2>/dev/null
/usr/bin/killall -KILL "WirelessConfig" 2>/dev/null
/usr/bin/killall -KILL -c "System Preferences" 2>/dev/null

if [ -e "/System/Library/Extensions/WirelessDriver.kext" ]; then
    if /usr/sbin/kextstat -b org.noncontiguous.WirelessDriver | /usr/bin/grep --quiet org.noncontiguous.WirelessDriver ; then
        "/sbin/kextunload" "/System/Library/Extensions/WirelessDriver.kext"
    fi
fi
