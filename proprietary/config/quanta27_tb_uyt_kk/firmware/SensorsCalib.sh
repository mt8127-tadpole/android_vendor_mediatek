#!/system/bin/sh
# 
# Program:
#		This program is using to Calibrate sensors
# 
# History: 2013/12/05	Josh.Wu@quantatw.com First release
IFS=","
export IFS;

function delay(){
    n=$1
        n2=$1
    while [ $n != 0 ]
    do
            while [ $n2 != 0 ]
                do
                        n2=$(($n2-1))
                done

        n=$(($n-1))
    done
}

echo "=== Start GSensor Calibration ==="

# Gsensor Calibration
defkadc=26
cnt=1
flag=0
xoffset=0
yoffset=0
zoffset=0
if [ $(test -e /data/misc/acdapi/gsensors.dat && echo "1" || echo "0") == 1 ]; then
	offset=$(cat /data/misc/acdapi/gsensors.dat)
	for offset in $offset
	do    
		test $cnt -eq 1 && echo "flag = 0x$offset" && flag=$offset
		test $cnt -eq 2 && echo "x = 0x$offset" && xoffset=$offset
		test $cnt -eq 3 && echo "y = 0x$offset" && yoffset=$offset
		test $cnt -eq 4 && echo "z = 0x$offset" && zoffset=$offset
		cnt=$(($cnt+1))
	done
else
	echo "Not find gsensors.dat!"
fi
if [ $flag == 1 ]; then
		echo w 0x38 0x$xoffset > /sys/devices/platform/gsensor/driver/iicrw
		delay 1000
		echo w 0x39 0x$yoffset > /sys/devices/platform/gsensor/driver/iicrw
		delay 1000
		echo w 0x3A 0x$zoffset > /sys/devices/platform/gsensor/driver/iicrw
		delay 1000
else
	echo "No manual calibration"
fi