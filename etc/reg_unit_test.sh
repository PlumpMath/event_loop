
#!/bin/bash
echo "Bash version ${BASH_VERSION}..."
ARG1=${1:-00}

NUMCOUNT=200000
PRINTEVERY=10
# for i in {0..2000..10}
for (( i=0;i<$NUMCOUNT;i++ ))
 do
     echo "Welcome $i times"
	for j in {0..1000..1}
	do
     		# echo "Welcome $i : $j times"
		./unit_test.sh $ARG1
		TESTRET=$?
		echo "end with:" $TESTRET
		if [ $TESTRET -eq 0 ]; then
			echo "GOOD.."
		else
			exit 1
		fi
	done
 done
echo "all reg unit_test END..."
exit 0
