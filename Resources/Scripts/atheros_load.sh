#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

if [ -e "/System/Library/Extensions/OMI_80211g.kext" ]; then
    if /usr/sbin/kextstat -b com.orangeware.iokit.OMI_80211g | /usr/bin/grep --quiet com.orangeware.iokit.OMI_80211g ; then
        /usr/bin/killall WirelessConfigurationService
        /usr/bin/killall OMI_80211g_App
        /bin/sleep 1
        /sbin/kextunload "/System/Library/Extensions/OMI_80211g.kext"
        /bin/sleep 2
    fi
fi

/usr/sbin/chown -R root:wheel "$LOCPATH/AtheroJack.kext"
/bin/chmod -R g-w "$LOCPATH/AtheroJack.kext"
/bin/chmod -R o-wrx "$LOCPATH/AtheroJack.kext"

/sbin/kextload "$LOCPATH/AtheroJack.kext"
