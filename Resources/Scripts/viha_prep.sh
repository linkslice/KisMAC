#!/bin/sh

if [ -e "/System/Library/Extensions/AppleAirPort.kext" ]; then
	/sbin/kextunload -b com.apple.driver.AppleAirPort
fi
