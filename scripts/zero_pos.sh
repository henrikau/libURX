#!/bin/bash 
if [[ -z $1 ]]; then
   echo "need argument (IP)"
fi
echo "Setting speed to 0"
echo "speedj([0.0, 0.0, 0.0, 0.0, 0.0, 0.0], 0, 1)" | nc ${1} 30002 | head -n1 > /dev/null
sleep 1
echo "Moving back to 0-pos"
echo "movej([0.0, -1.57, 0.0, -1.57, 0.0, 0.0], a=2, v=20, r=0)" | nc ${1} 30002 | head -n1 > /dev/null