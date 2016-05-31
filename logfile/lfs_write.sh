#!/bin/bash
echo "Bash version ${BASH_VERSION}..."
NUMCOUNT=200000
PRINTEVERY=10
# for i in {0..2000..10}
for (( i=0;i<$NUMCOUNT;i++ ))
 do
     echo "Welcome $i times"
	for j in {0..1000..1}
	do
     		# echo "Welcome $i : $j times"
	     ./lfs_test $1
	done
 done
