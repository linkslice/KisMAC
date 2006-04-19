#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/sbin/kextunload "$LOCPATH/Insomnia.kext"
