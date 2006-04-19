#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/usr/sbin/chown -R root:wheel "$LOCPATH/WLanDriver.kext"
/bin/chmod -R g-w "$LOCPATH/WLanDriver.kext"
/bin/chmod -R o-wrx "$LOCPATH/WLanDriver.kext"

/sbin/kextload "$LOCPATH/WLanDriver.kext"
