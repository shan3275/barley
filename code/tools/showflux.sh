# cat stream

#! /bin/bash  
# Write by Neil.xu  qq:37391319 email: xurongzhong@gmail.com  
# 2008-8-19 we need to monitor streams of LTS channels, so write this script  

typeset in in_old dif_in dif_in1 dif_out1  
typeset out out_old dif_out  

if [ $# -ne 1 ]
then
    echo ""
    echo "$0 octX"
    echo ""
    exit
fi

time=3

ifname=$1
#
ifnum=`cat /proc/net/dev  | grep -c $ifname`
if [ $ifnum -eq 0 ]
then
    echo ""
    echo "device [$ifname] not found"
    echo ""
    exit
fi

#stat
speed=`ethtool $ifname |grep Speed`
link=`ethtool $ifname |grep 'Link detected'|awk '{print $3}'`
running=`ifconfig $ifname |grep -c RUNNING`
promisc=`ifconfig $ifname |grep -c PROMISC`
if [ $running -eq 0 ]
then
    run_stat="no"
else
    run_stat="yes"
fi
if [ $promisc -eq 0 ]
then
    run_mode="no"
else
    run_mode="yes"
fi

rx_byte_old=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $1 }')  
rx_pkts_old=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $2 }')

tx_byte_old=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $9 }') 
tx_pkts_old=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $10 }')

while true  
do  
         sleep $time
         rx_byte_new=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $1 }')
         rx_pkts_new=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $2 }')
         rx_drop=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $4 }')
         tx_byte_new=$(cat /proc/net/dev | grep $ifname | sed  's=^.*:=='  | awk '{ print $9 }')       
         tx_pkts_new=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $10 }')
         tx_drop=$(cat /proc/net/dev  | grep $ifname | sed  's=^.*:=='  | awk '{ print $12 }')
         delta_rx_byte=$((rx_byte_new-rx_byte_old))
         delta_rx_pkts=$((rx_pkts_new-rx_pkts_old))
         rx_mbps=$((delta_rx_byte * 8 / 1024 / 1024 / $time))
         rx_kpps=$((delta_rx_pkts/1024/$time))
         delta_tx_byte=$((tx_byte_new-tx_byte_old))
         delta_tx_pkts=$((tx_pkts_new-tx_pkts_old))
         tx_mbps=$((delta_tx_byte * 8 / 1024 / 1024 / $time))
         tx_kpps=$((delta_tx_pkts/1024/$time))
         echo ""
         echo "$ifname: $speed, link:$link, running:$run_stat, promisc:$run_mode, print:$time sec"
         echo "Rx: ${rx_mbps} mbps, ${rx_kpps} kpps, drop ${rx_drop}"
         echo "Tx: ${tx_mbps} mbps, ${tx_kpps} kpps, drop ${tx_drop}"  
         echo ""
         rx_byte_old=${rx_byte_new}
         rx_pkts_old=${rx_pkts_new}
         tx_byte_old=${tx_byte_new}
         tx_pkts_old=${tx_pkts_new}
done  
