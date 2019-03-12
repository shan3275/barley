#!/bin/sh
module=frc
device=frc
mode="666"
ifconfig oct0 down
ifconfig oct1 down

rmmod frcdrv
rmmod octeon_drv
sleep 3
insmod octeon_drv.ko
sleep 3
./oct-pci-load 0x20000000 frcore.strip
sleep 1
./oct-pci-bootcmd "bootoct 0x20000000 coremask=FFF"
sleep 5
insmod ./frcdrv.ko

# retrieve major number
major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)

# Remove stale nodes and replace them, then give gid and perms
# Usually the script is shorter, it's c531drv that has several devices in it.

rm -f /dev/${module}
mknod /dev/${module} c $major 0

ifconfig oct0 up
ifconfig oct1 up
ifconfig oct0 192.168.1.2/24 mtu 1600 promisc
ifconfig oct1 192.168.2.2/24 mtu 1600 promisc

cp barley /sbin/
./barley board info

