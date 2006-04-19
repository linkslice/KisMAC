#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/usr/bin/killall -KILL "SigMeter" 2>/dev/null
/usr/bin/killall -KILL -c "System Preferences" 2>/dev/null

/bin/sleep 1

if [ -e "/System/Library/Extensions/IOXperts80211.kext" ]; then
    if /usr/sbin/kextstat -b com.ioxperts.iokit.80211 | /usr/bin/grep --quiet com.ioxperts.iokit.80211 ; then
        "/sbin/kextunload" "/System/Library/Extensions/IOXperts80211.kext"
    fi
fi

if [ -e "/System/Library/Extensions/WirelessDriver.kext" ]; then
    if /usr/sbin/kextstat -b org.noncontiguous.WirelessDriver | /usr/bin/grep --quiet org.noncontiguous.WirelessDriver ; then
        "/sbin/kextunload" "/System/Library/Extensions/WirelessDriver.kext"
    fi
fi

if [ -e "/System/Library/Extensions/AeroCard.kext" ]; then
    if /usr/sbin/kextstat -b com.macsense.iokit.AeroCard | /usr/bin/grep --quiet com.macsense.iokit.AeroCard ; then
        "/sbin/kextunload" "/System/Library/Extensions/AeroCard.kext"
    fi
fi

#due a bug the cisco card driver gets loaded sometimes
if [ -e "/System/Library/Extensions/CiscoPCCardRadio.kext" ]; then
    if /usr/sbin/kextstat -b com_cisco_PCCardRadio | /usr/bin/grep --quiet com_cisco_PCCardRadio ; then
        /sbin/kextunload "/System/Library/Extensions/CiscoPCCardRadio.kext"
    fi
fi

/usr/sbin/chown -R root:wheel "$LOCPATH/MACJack.kext"
/bin/chmod -R g-w "$LOCPATH/MACJack.kext"
/bin/chmod -R o-wrx "$LOCPATH/MACJack.kext"
/bin/sleep 1

/sbin/kextload "$LOCPATH/MACJack.kext"
