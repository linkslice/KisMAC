#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

if [ -e "/System/Library/Extensions/AppleAirPort.kext" ]; then
        /sbin/kextunload "/System/Library/Extensions/AppleAirPort.kext"
fi
