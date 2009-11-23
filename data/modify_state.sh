#!/bin/sh
if [ $# -ne 3 ]
then
	echo "Usage: $0 <state> <control name> <value to set>"
	return 1
fi

STATEPATH="/etc/freesmartphone/alsa/default"

#in case it's a mono control
sed -i "s/\('$2'\):1:[0-9]\+/\1:1:$3/" ${STATEPATH}/$1
#in case it's a stereo control
sed -i "s/\('$2'\):2:[0-9]\+,[0-9]\+/\1:2:$3,$3/" ${STATEPATH}/$1
