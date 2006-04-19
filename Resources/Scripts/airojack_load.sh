#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/usr/bin/killall "Aironet Client Utility" 2>/dev/null
/usr/bin/killall "Aironet Status Utility" 2>/dev/null

if [ -e "/System/Library/Extensions/CiscoPCCardRadio.kext" ]; then
    if /usr/sbin/kextstat -b com_cisco_PCCardRadio | /usr/bin/grep --quiet com_cisco_PCCardRadio ; then
        /sbin/kextunload "/System/Library/Extensions/CiscoPCCardRadio.kext"
    fi
fi

/usr/sbin/chown -R root:wheel "$LOCPATH/AiroJack.kext"
/bin/chmod -R g-w "$LOCPATH/AiroJack.kext"
/bin/chmod -R o-wrx "$LOCPATH/AiroJack.kext"
/bin/sleep 1

/sbin/kextload "$LOCPATH/AiroJack.kext"
