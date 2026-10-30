#!/bin/bash


for i
do
while read line #reads each line and runs it thru the issueJob command
	do
    ./jobCommander issueJob $line
	done < $i
done