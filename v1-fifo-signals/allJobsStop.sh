#!/bin/bash

# Running "poll queued" and redirecting the output to the output.txt file
./jobCommander poll queued > output.txt

awk -F'[<,]' '{print $2}' output.txt | while read -r job_id; do #getting the id's from each line of code "printed" on the output.txt file
	if [ -n "$job_id" ]; then
    	./jobCommander stop "$job_id" #the running the sto stop command for each job_id printed
    fi
done

#doing the same for the running jobs-jobIDs
./jobCommander poll running > output1.txt

awk -F'[<,]' '{print $2}' output1.txt | while read -r job_id; do
    if [ -n "$job_id" ]; then
    	./jobCommander stop "$job_id"
    fi
done

