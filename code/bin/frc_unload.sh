#!/bin/sh
ifconfig oct0 down
ifconfig oct1 down

rmmod frcdrv
rmmod octeon_drv
