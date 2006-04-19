#!/bin/sh

ROOT_DIR="$1"
PACKAGE_DIR="$2"

cd $ROOT_DIR
if /usr/bin/cmp "$PACKAGE_DIR/WirelessDriver" "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver">/dev/null; then
    /bin/echo "The WirelessDriver is already the newest version. No need to overwrite."
else 
    /bin/cp -R "$PACKAGE_DIR/WirelessDriver" "/System/Library/Extensions/WirelessDriver.kext/Contents/MacOS/WirelessDriver"
    /usr/sbin/kextcache -L -N -k -e
fi

/usr/bin/touch "/System/Library/Extensions/WirelessDriver.kext/bipatch"
