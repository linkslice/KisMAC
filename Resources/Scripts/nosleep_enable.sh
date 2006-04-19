#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/usr/sbin/chown -R root:wheel "$LOCPATH/Insomnia.kext"
/bin/chmod -R g-w "$LOCPATH/Insomnia.kext"
/bin/chmod -R o-wrx "$LOCPATH/Insomnia.kext"

/sbin/kextload "$LOCPATH/Insomnia.kext"

