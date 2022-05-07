##!/bin/bash

broker="test.mosquitto.org"
port="1883"
topic1="tp-final/Hanes/ESP32"
topic2="tp-final/Hanes/App"
file="Log.txt"

#--------------------------------------------------
if [ -f $file ]; then \
    rm -rf $file
fi

touch $file

mosquitto_sub -t $topic1 -t $topic2 -h $broker -p $port -v | while read topic value; do
#Add timestamp on the log
timestamp=$(date "+%d/%m/%Y %H:%M:%S")

if [ $topic == $topic1 ]; then \
if [ "$value" == "Reset" ]; then \
    echo "$timestamp MQTT-APP: $topic $value." >> $file
else
    echo "$timestamp MQTT-NODO: $topic $value." >> $file
fi
else
    echo "$timestamp MQTT-APP: $topic $value." >> $file
fi

done
