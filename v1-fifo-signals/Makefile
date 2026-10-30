reset:
	rm -f fifo_1 fifo_2 jobExecutorServer.txt

jobCommander : jobCommander.o jobExecutorServer.o DequeImplementation.o
	gcc -o jobCommander jobCommander.o jobExecutorServer.o DequeImplementation.o

jobCommander.o : jobCommander.c
	gcc -c jobCommander.c

jobExecutorServer.o : jobExecutorServer.c
	gcc -c jobExecutorServer.c

DequeImplementation.o : DequeImplementation.c
	gcc -c DequeImplementation.c

progDelay : progDelay.o
	gcc -o progDelay progDelay.o

progDelay.o : progDelay.c
	gcc -c progDelay.c

clean : 
	rm -f *.o

multijob:
	chmod +x multijob.sh

allJobsStop:
	chmod +x allJobsStop.sh