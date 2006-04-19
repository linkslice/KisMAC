#!/bin/sh

ROOT_DIR="$3"

if [ -e "/System/Library/Extensions/WirelessDriver.kext" ]; then
	/usr/sbin/chown -R root:wheel "/System/Library/Extensions/WirelessDriver.kext"
fi
