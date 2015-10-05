#!/bin/bash

RPIPE=/var/tmp/led_r.fifo
WPIPE=/var/tmp/led_w.fifo
MENU="Select LED command:\n 1. Get state\n 2. Get rate\n 3. Get color\n 4. Set state\n 5. Set rate\n 6. Set color\n q. Exit script\n"
while true;
do
clear
echo -e $MENU
read -p ">" I1
if   [[ "$I1" = "1" ]] 
then
	CMD=get-led-state
elif [[ "$I1" = "2" ]] 
then
	CMD=get-led-rate
elif [[ "$I1" = "3" ]]
then
	CMD=get-led-color
elif [[ "$I1" = "4" ]]
then
	clear
	echo -e $MENU
	echo "Input state: on/off"
	CMD=set-led-state
	read -p ">" ARG
elif [[ "$I1" = "5" ]]
then	
	clear
	echo -e $MENU
	echo "Input rate: 0..5 (Hz)"
	CMD=set-led-rate
	read -p ">" ARG
elif [[ "$I1" = "6" ]]
then
	clear	
	echo -e $MENU
	echo "Input color: red/green/blue"
	CMD=set-led-color
	read -p ">" ARG
elif [[ "$I1" = "q" ]]
then
	clear
	exit
else
	clear
	echo -e $MENU
	echo "wrong command"
	sleep 1
	continue
fi
echo "$CMD $ARG" >>$RPIPE
read line <$WPIPE
clear
echo -e $MENU
if [[ $line = "FAILED" ]]
then
	echo "Command failed!"
else
	echo "Command succeeded! Result:" ${line:2}
fi
sleep 2
done
