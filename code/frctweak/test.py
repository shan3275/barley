#!/usr/bin/python
#Filename:test.py

import time
import os

while True :
	print time.ctime()
	os.system('sudo ./frctweak board info')
	print time.ctime()
	print '\n'
	#time.sleep(1)

