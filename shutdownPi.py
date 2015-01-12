#!/usr/bin/python
# -*- coding: utf-8 -*-
# *******************************shutdownPi*******************************
# 28.03.2014 Selim Olcer 
# 

import time
import RPi.GPIO as GPIO
import os


def shutdown(buton):
	print "bye bye..."
	time.sleep(0.05)
	if (GPIO.input(buton) ==0):
		#os.system("cd /home/pi/selim/bmp2fb")
		os.system("sudo ./cleanFBs")
		#time.sleep(1)
	    	os.system("sudo shutdown -h now")


#command = 'screen -d -m /home/pi/selim/bmp2fb/./staticGoruntu'
#print command
#os.system(command)
#command = './shutdownPi'
#print command
#os.system(command)
buton = 11										#Buton'un baglanacagi pin numarasi
GPIO.setmode(GPIO.BOARD)
GPIO.setup(buton,GPIO.IN,pull_up_down = GPIO.PUD_UP)				#buton'u input olarak tanýmlýyorum. Pull up direncini aktif ediyorum.
GPIO.add_event_detect(buton, GPIO.FALLING, callback=shutdown, bouncetime=50)	#dusen kenarda 200ms bekleyip shutdown fonksiyonuna gidiyor
while True:
	time.sleep(.2)								#sonsuz dongu
