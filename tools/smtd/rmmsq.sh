#!/bin/bash 



ipcs -q | while read line
do
	line=${line//\t/ }
	for elmt in $line
	do
		if [[ $elmt == "0x"* ]]; then
			echo "ipcrm -Q $elmt"
			ipcrm -Q $elmt
		fi
	done
done
