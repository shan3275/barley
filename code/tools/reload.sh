#!/bin/sh

MAX=100

if [ "$#" = "1" ];then
MAX=$1
fi

ERR=0
CNT=0
NIC=eth0
echo "Start reload frc driver $MAX timers at `date +%T`"
start=`date +%s`
while [ $CNT -lt $MAX ]
do
    printf "Round %3d: ...... " $CNT
    rx_pkts=0
    t1=`date +%s`
    ./frc.sh > /dev/null
    
    if [ $? == 0 ]; then
        cat /proc/net/dev|grep $NIC > /dev/null
        #echo "\$? = $?"
        if [ $? == 0 ]; then
            rx_pkts=`cat /proc/net/dev|grep $NIC|awk '{print $2}'`
            if [ $rx_pkts == 0 ]; then
                FAIL=1
            else
                FAIL=0
            fi
        else
            FAIL=1
        fi 
        
    else
        FAIL=1
    fi 

    t2=`date +%s`
    t3=`expr $t2 - $t1`

    if [ $FAIL == 0 ]; then
        printf " DONE. Used %.2d seconds: $NIC rx %5d packets.\n" $t3 $rx_pkts
    else
        printf " FAIL! Used %.2d seconds: $NIC rx %5d packets.\n" $t3 $rx_pkts
        ERR=`expr $ERR + 1`
    fi

    CNT=`expr $CNT + 1`
done

end=`date +%s`
spend=`expr $end - $start`
sec=`expr $spend % 60`
min=`expr $spend / 60`

printf "Reload frc %d timres completed at `date +%T`. Spend %.2d:%.2d, %d errors found.\n" $CNT  $min $sec $ERR
