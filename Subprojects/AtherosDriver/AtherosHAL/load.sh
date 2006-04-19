#!/bin/sh

#DEST=build/IO80211FamilyRoot.kext
DEST=/System/Library/Extensions/AtherosHAL.kext
kextunload $DEST
rm -rf $DEST
cp -r build/AtherosHAL.kext $DEST
chown -R root $DEST
chgrp -R wheel $DEST
sudo rm /System/Library/Extensions.kextcache
kextcache -k /System/Library/Extensions
KEXTD=`ps -x -Uroot | grep kextd | awk '{print $1}'`
kill -HUP $KEXTD
kextload $DEST
