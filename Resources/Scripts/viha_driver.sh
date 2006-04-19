#!/bin/sh

DRIVER_ID=org.theta44.iokit.WLanDriver
APDRIVER_ID=com.apple.driver.AppleAirPort
AIRPORT_KEXT=/System/Library/Extensions/AppleAirPort.kext
AIRPORT_PROG=`dirname "$0"`/AirPortMenu
DRIVER_KEXT=`dirname "$0"`/WLanDriver.kext
STATUS=`/usr/sbin/kextstat -lb $DRIVER_ID`
APSTATUS=`/usr/sbin/kextstat -lb $APDRIVER_ID`

unload_airport() {
    LOAD=n
    echo "$0 unloading Airport driver"
    #/sbin/ifconfig en1 down
    if [ "x$APSTATUS" != "x" ]; then
        /sbin/kextunload $AIRPORT_KEXT
        if [ $? != 0 ]; then
            echo "$0: Could not unload Apple AirPort driver"
            exit 1
        fi
    fi
}

load_airport() {
    LOAD=y
    echo "$0 loading Airport driver"
    if [ -x $AIRPORT_KEXT ]; then
        if [ "x$APSTATUS" == "x" ]; then
            /sbin/kextload $AIRPORT_KEXT
            if [ $? != 0 ]; then
                echo "$0: Could not load Apple AirPort driver"
                exit 1
            fi
        fi
    fi
    /sbin/ifconfig en1 up
}

kick_menu() {
    #/usr/bin/killall SystemUIServer
    if [ "$LOAD" != "y" ]; then
        "$AIRPORT_PROG" stop
    else
        "$AIRPORT_PROG" start
    fi
}

load_driver() {
    if [ "x$STATUS" == "x" ]; then
        unload_airport
        /bin/sleep 1
        kick_menu
        /usr/sbin/chown -R root:wheel "$DRIVER_KEXT"
        /bin/chmod -R g-w "$DRIVER_KEXT"
        /bin/chmod -R o-wrx "$DRIVER_KEXT"
        /sbin/kextload "$DRIVER_KEXT"
        if [ $? != 0 ]; then
            echo "$0: Could not load WLanDriver"
            exit 1
        fi
    else
        echo "$0: WLanDriver already loaded"
    fi
}

reload_driver() {
     /sbin/kextunload "$DRIVER_KEXT"
     /sbin/kextload "$DRIVER_KEXT"
}

unload_driver() {
    /bin/sleep 2
    
    if [ "x$STATUS" != "x" ]; then
        /sbin/kextunload "$DRIVER_KEXT"
        if [ $? != 0 ]; then
            echo "$0: Could not unload WLanDriver"
            exit 1
        fi
    else
        echo "$0: WLanDriver not loaded"
    fi
    
    /bin/sleep 2
    load_airport
    kick_menu
}

if [ $# -lt 1 ]; then
    echo "usage: $0 { start | stop }"
    exit 1
fi

if [ ! -d "$DRIVER_KEXT" ]; then
    echo "$0: WLanDriver kernel extension not found"
    exit 1
fi

if [ ! -d $AIRPORT_KEXT ]; then
    echo "$0: Apple AirPort kernel extension not found"
    exit 1
fi

case $1 in
    start)
        load_driver
        ;;
    stop)
        unload_driver
        ;;
    restart)
        reload_driver
        ;;
    *)
        echo "$0: unknown argument $1"
        ;;
esac

exit 0
