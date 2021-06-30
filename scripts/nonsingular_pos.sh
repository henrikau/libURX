#!/bin/bash 
if [[ -z $1 ]]; then
   echo "no IP provided, using 10.218.180.29"
   IP="10.218.180.29"
else
   IP=${1}
fi
echo "Setting speed to 0"
echo "speedj([0.0, 0.0, 0.0, 0.0, 0.0, 0.0], 0, 1)" | nc $IP 30002 | head -n1 > /dev/null
sleep 1
echo "Moving to a nonsingular pos"
echo "movej([2, -1.4, 2, -3.7, -1.75, -1.57], a=2, v=20, r=0)" | nc $IP 30002 | head -n1 > /dev/null