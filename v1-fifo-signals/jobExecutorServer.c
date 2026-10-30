#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "variables.h"
#include "DequeInterface.h"
int N;
#define NEW 0
#define QUEUED 1
queued_Deque Q_D;	//declared here so they can be used in sig_handler
running_Deque R_D;

void split_to_arg(char *param, char **vp_arguments) { // constructs the arguments for the execvp() functions
    while (*param != '\0') {
        while (*param == ' '||*param == '\t' || *param == '\n') {
            *param++ = '\0';
        }
        if (*param != '\0') {
            *vp_arguments++ = param;
        }
        while (*param != '\0' && *param != ' ' && *param != '\t' && *param != '\n') {
            param++;
        }
    }
    *vp_arguments = NULL;  //marking the end of the array
}

void send_to_commander(char *str_to_comm) { //send message back to commander to be printed
	int write_fd;
	char write_str[max_command_size];
	if( (write_fd=open(fifo2, O_WRONLY)) < 0) {
			printf("\ncan't open-write fifo2\n");
			return;
		}
		strcpy(write_str, str_to_comm);
		if (write(write_fd, write_str, max_command_size) == -1) {
			printf("\nCan't write to fifo2\n");
			return;
		}
		write_str[0] = '\0';
		strcpy(write_str, "close");
		if (write(write_fd, write_str, max_command_size) == -1) {
			printf("\nCan't write to fifo2\n");
			return;
		}
		close(write_fd);
}

void issueJob(char *,char *,int,queued_Deque *,running_Deque *);

void update() { //updates the running and queued jobs based on the concurrency that was inputed
	char job_print[max_command_size];
	char jobID[100];
	
	int run_jobs = run_SizeOf(&R_D);
	if (run_jobs < N) {
		for (int i=0 ; i < (N-run_jobs) ; i++) {
			int que_jobs = que_SizeOf(&Q_D);
			if (que_jobs > 0) {
				strcpy(job_print , (&Q_D)->Front->job);
				strcpy(jobID, (&Q_D)->Front->jobID);
				que_Pop(&Q_D);
				issueJob(jobID,job_print,QUEUED,&Q_D, &R_D);
			}
		}
	}
}

void sig_handler(int signal_1) {
	int stat_loc,pr_pos;
	pid_t id;
	int run_jobs;
	char job_id[max_command_size],job[max_command_size];
	while ((id = waitpid(-1, &stat_loc, WNOHANG)) > 0) { //once a job is done running then it removes it from the running queue
		run_eliminating_pid(id,&R_D);
	}
	run_jobs = run_SizeOf(&R_D);

	for(int i = 0 ; i < (N-run_jobs) ;i++) { //it then enqueues the waiting jobs
		int que_jobs = que_SizeOf(&Q_D);
		if(que_jobs > 0 ) {
			strcpy(job_id,(&Q_D)->Front->jobID);
			strcpy(job,(&Q_D)->Front->job);
			pr_pos = (&Q_D)->Front->quequePositon;
			que_Pop(&Q_D);
			issueJob(job_id,job,QUEUED,&Q_D,&R_D);
		}
	}
}

void issueJob(char *jobID,char *parameter,int new_or_queued,queued_Deque *Q_D_J,running_Deque *R_D_J) {
	char *exec_file;
	pid_t id;
	int pr_pos;
	char str_to_comm[max_command_size],job_param[max_command_size],job_param2[max_command_size];
	char *vp_arguments[100];
	strcpy(job_param,parameter); //job_param stays untouched after use of strtok function
	exec_file = strtok(parameter, " ");

	signal(SIGCHLD, sig_handler);

	int run_jobs = run_SizeOf(R_D_J);
	char exec_str[max_command_size];
	if(access(exec_file,X_OK) == 0) {
  		sprintf(exec_str,"./%s",exec_file);
	} 
	else {
  		sprintf(exec_str,"%s",exec_file);
	}
	strcpy(job_param2,job_param); //copyig the string cause it is gong to get messed by splitting the string to arguments
	split_to_arg(job_param2,vp_arguments);
	
	if(run_jobs < N ) {
		id = fork();
		if(id == -1) {
			printf("\nServer wasn't created , fork did't work !\n");
		}
		else if(id == 0) { //child //fork worked
			parameter = strtok(NULL, "");
			execvp(exec_str,vp_arguments); //runs the job
		}
		else {
			run_Enqueue(id,jobID,job_param,R_D_J); //puts the running job into the running queue
			if(new_or_queued == NEW) { //if it is new it also prints it!
				pr_pos = R_D_J->Rear->quequePositon;
				sprintf(str_to_comm,"\n<%s, %s ,%d>\n", jobID,job_param,pr_pos);
				send_to_commander(str_to_comm);
			}
		}
	}
	else {
		que_Enqueue(jobID,job_param,Q_D_J); //the job can't be run right right away , so it gets put into queued jobs queue and it also prints it! 
		pr_pos = Q_D_J->Rear->quequePositon;
		sprintf(str_to_comm,"\n<%s, %s ,%d>\n", jobID,job_param,pr_pos);
		send_to_commander(str_to_comm);		
	}
}

int jobExecutorServer() {
    int job_id = 1,flag, read_fd, i;
	N = 1;
    char read_str[max_command_size],str_to_comm[max_command_size] , *comm , *parameter, jobID[10] ;
	run_Create(&R_D);
	que_Create(&Q_D);

	if (mkfifo (fifo2 , 0666) == -1){ //creating fifo2
		if (errno != EEXIST ) {
			printf("\ncouldn't make fifo1\n");
			return -10;
		}
	}

    while(1) {
		str_to_comm[0] = '\0';
		flag = 0;

		if((read_fd = open(fifo1, O_RDWR)) < 0) {
			printf("\ncan't open-write fifo1\n");
			return -11;	
		}

		if(read(read_fd, read_str, max_command_size ) < 0) { //gets the arguments from the ./jobCommander ....
			printf("\nCan't read from fifo2\n");
			return -12;
		}
		close(read_fd);

		fflush(stdout);
		comm = strtok(read_str, " ");
		if(strcmp(comm, "issueJob") == 0){
			flag = 1;
			parameter = strtok(NULL, "");
			if (parameter == NULL)
				printf("\nissueJob is missing arguments\n");
			else { 
				sprintf(jobID,"job_%d",job_id);
				job_id++;
				issueJob(jobID,parameter,NEW,&Q_D,&R_D);
			}
		}
		else if(strcmp(comm, "setConcurrency") == 0) {
			flag = 1;
			parameter = strtok(NULL, " ");
			fflush(stdout);
			if (parameter == NULL) {
				sprintf(str_to_comm,"\nsetConcurrency : missing argument\n");
				send_to_commander(str_to_comm);
			}
			else {
				N = atoi(parameter);
				sprintf(str_to_comm,"\nconcurency set to %d\n",N);
				send_to_commander(str_to_comm);
				update(); //updates the running and queued jobs based on the concurrency that was inputed
			}
		}
		else if(strcmp(comm, "stop") == 0) {
			flag = 1;
			parameter = strtok(NULL, "");
			if(run_stop_jobID(parameter,&R_D) == 1) {
				sprintf(str_to_comm,"%s terminated\n",parameter);
				send_to_commander(str_to_comm);
			}
			else {
				if(que_stop_jobID(parameter,&Q_D) == 1) {
					sprintf(str_to_comm,"%s removed\n",parameter);
					send_to_commander(str_to_comm);
				}
				else {
					sprintf(str_to_comm,"%s wasn't found\n",parameter);
					send_to_commander(str_to_comm);
				}
			}
		}
		else if(strcmp(comm, "poll") == 0) { //prints either all the queued or running jobs
			flag = 1;
			parameter = strtok(NULL, " ");
			if(strcmp(parameter,"running")==0) {
				run_print(str_to_comm,&R_D);
			}
			else if (strcmp(parameter,"queued")== 0) {
				que_print(str_to_comm,&Q_D);
			}
			send_to_commander(str_to_comm);
		}
		else if(strcmp(comm, "exit") == 0) { //termintes the server
			flag = 1;
			remove(server_pid_file);
			sprintf(str_to_comm,"jobExecutorServer terminated\n");
			send_to_commander(str_to_comm);
			break;
		}
		
		if(flag == 0) {
			fflush(stdout);
			sprintf(str_to_comm,"\nFALSE OPERATION --> NOT: issuejob,setConcurrency,stop,exit\n");
			send_to_commander(str_to_comm);
		}
    }
}