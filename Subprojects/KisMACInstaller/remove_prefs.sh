#!/bin/sh

for USERS in `/bin/ls /Users`; do
	if [ -e "/Users/$USERS/Library/Preferences/org.aziel.kismac.plist" ]; then
		/bin/rm -f "/Users/$USERS/Library/Preferences/org.aziel.kismac.plist"
	fi
	if [ -e "/Users/$USERS/Library/Preferences/org.binaervarianz.kismac.plist" ]; then
		/bin/rm -f "/Users/$USERS/Library/Preferences/org.binaervarianz.kismac.plist"
	fi
	if [ -e "/Users/$USERS/Library/Preferences/de.binaervarianz.kismac.plist" ]; then
		/bin/rm -f "/Users/$USERS/Library/Preferences/de.binaervarianz.kismac.plist"
	fi
	if [ -e "/Users/$USERS/Library/Preferences/com.kismac-ng.kismac.plist" ]; then
		/bin/rm -f "/Users/$USERS/Library/Preferences/com.kismac-ng.kismac.plist"
	fi
        if [ -e "/Users/$USERS/Library/Logs/CrashReporter/KisMAC.crash.log" ]; then
		/bin/rm -f "/Users/$USERS/Library/Logs/CrashReporter/KisMAC.crash.log"
	fi
done	
