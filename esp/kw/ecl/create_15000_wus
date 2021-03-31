#!/bin/bash
limit=$1
##for number in {1..2}
for ((number=1;number <= $limit;number++))
do
echo "$number "
ecl run thor -u kwang -pw seisint1 t1.ecl
done
for ((number=1;number <= $limit;number++))
do
echo "$limit + $number "
ecl run thor -u kwang -pw seisint1 t2.ecl
done
exit 0
