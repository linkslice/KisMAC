#!/bin/sh

LOCPATH=`/usr/bin/dirname "$0"`

/bin/cp /mach_kernel /mach_kernel.backup
/bin/cp "$LOCPATH/other_kernel" /mach_kernel

/usr/sbin/chown root:wheel /mach_kernel
/bin/chmod 644 /mach_kernel

/sbin/reboot
