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
int max_command_size = 2048;
char *server_pid_file = "jobExecutorServer.txt";
char *fifo1 = "fifo_1"; //commander writes -> executor reads
char *fifo2 = "fifo_2"; //executor writes -> commander reads

void jobExecutorServer();

int main(int argc, char *argv[]) {

    if(argc == 1) {
        printf("\ncommander::::missing arguments!!\n");
        return -2;
    }

    FILE *server = fopen(server_pid_file,"r");
    int write_fd,read_fd;
    char write_arg[max_command_size],read_arg[max_command_size];
    if(server == NULL) {
        pid_t server_pid = fork();
        if(server_pid == -1) {
            printf("\nServer wasn't created . Fork did't work !!\n");
            return -1;
        }
        else if(server_pid == 0) { //child //fork worked and server was created saving the pid of the server to the server_pid_file
            server = fopen(server_pid_file,"w");
            fprintf(server,"%d\n",getpid());
            fclose(server);
            jobExecutorServer(); //server is activated
            remove(fifo1);
            remove(fifo2);
            return 0;
        }
    }

    if(argc >= 2) { //formats the command-parameter string to be passed into 
        strcpy(write_arg,argv[1]);
        for(int i =2;i < argc && argv[i]!=NULL;i++) {
            sprintf(write_arg, "%s %s", write_arg, argv[i]);
        }
    }

	if (mkfifo(fifo1 , 0666) ==-1) { //creating fifo1
		if (errno != EEXIST ) {
	    	printf("\ncouldn't make fifo1\n");
            return -2;
		}
	}

	if( (write_fd = open(fifo1, O_WRONLY)) < 0) {
		printf("\ncan't open-write fifo1\n");
		return -3;
	}

    if (write(write_fd, write_arg, max_command_size) == -1) { //writting to server
		printf("\nCan't write to fifo1\n");
		return -4;
	}
    close(write_fd);

	if ( (read_fd=open(fifo2, O_RDWR)) < 0) {
		printf("\nCan't open-read fifo2\n"); 
		return -5;	
	}

	while (1) {//reads executors message until it recieves "close" string
		if (read(read_fd, read_arg , max_command_size ) < 0) {
			printf("\nCan't read from fifo2\n");
            return -6;
		}
		if (strcmp(read_arg, "close") == 0) {
            break;
        }
		printf("%s\n", read_arg);
	}
	close(read_fd);

    return 0;
}