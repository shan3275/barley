#!/bin/sh
if [ -z "$1" ]
then
	echo "Usage: $0 PATH FILE"
	exit
fi

if [ -z "$2" ]
then
	echo "Usage: $0 PATH FILE"
	exit
fi

a=0
RELOAD_SUCCESS=""
while [ "1" ]
do
FRC_STATUS=`cat /proc/Octeon0/frc_status`
if [ "$FRC_STATUS" = "ok" ]
then
	sleep 1
	continue
elif [ "$FRC_STATUS" = "unstable" ]
then
	killall tcpreplay
	cd $1;PATH=`pwd`:$PATH;oct-pci-reset;sleep 1;./$2
#	let a++
    RELOAD_SUCCESS=`ifconfig|grep oct0`
    if [ -z "$RELOAD_SUCCESS"]
    then
        ./pcie_control_reset.sh
         cd $1;PATH=`pwd`:$PATH;oct-pci-reset;sleep 1;./$2    
    fi
	echo $(date) >>$1/recover_log.txt
fi
sleep 1
done
