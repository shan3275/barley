#!/bin/sh

MAX=10000000

if [ "$#" = "1" ];then
MAX=$1
fi

start_time=`date +%s`

CNT=0
spend=0
min=0
sec=0
dump_time=0
tmp=""
rx_pkts_now=0
rx_pkts_old=0
rx_pkts_spd=0
tx_pkts_now=0
tx_pkts_old=0
tx_pkts_spd=0

while [ $CNT -lt $MAX ]
do
    CNT=`expr $CNT + 1`
    now=`date +%s`
    spend=`expr $now - $start_time`
    sec=`expr $spend % 60`
    min=`expr $spend / 60`
    
    tmp=`frctweak temp|awk '{print $6}'`
    rx_pkts_now=`frctweak stat all|grep p1_rx_pkts|awk '{print $3}'`
    tx_pkts_now=`frctweak stat all|grep p1_tx_pkts|awk '{print $3}'`

    if [ $spend -gt $dump_time ]; then
        if [ $rx_pkts_old -gt 0 ] && [ "$rx_pkts_now" != "" ]; then
            rx_pkts_spd=`expr $rx_pkts_now - $rx_pkts_old`
            rx_pkts_spd=`expr $rx_pkts_spd / 1000`
            rx_pkts_old=$rx_pkts_now
        fi

        if [ $tx_pkts_old -gt 0 ] && [ "$tx_pkts_now" != "" ]; then
            tx_pkts_spd=`expr $tx_pkts_now - $tx_pkts_old`
            tx_pkts_spd=`expr $tx_pkts_spd / 1000`
            tx_pkts_old=$tx_pkts_now
        fi

        printf "\r%8d: TMP %3s RX %-12s %4dKpps TX %-12s %4dKpps [%.4d:%.2d]" \
        $CNT $tmp $rx_pkts_now $rx_pkts_spd $tx_pkts_now $tx_pkts_spd $min $sec

        dump_time=$spend
    fi
#    usleep 10000
done

printf "\r%8d: TMP %3s RX %-12s %4dKpps TX %-12s %4dKpps [%.4d:%.2d]\n" \
       $CNT $tmp $rx_pkts_now $rx_pkts_spd $tx_pkts_now $rx_pkts_spd $min $sec 

