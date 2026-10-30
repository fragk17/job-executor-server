#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "DequeInterface.h"
#define REQUESTS 100
#include "variables.h"

pthread_mutex_t mutex;
pthread_cond_t cond_que;

queued_Deque QD;

volatile sig_atomic_t terminate = 0;
pthread_t main_thread_id;

int active_workers = 0;
int threadPoolSize;
int server_socket;

//initializing mutex and condition variable
int initialize() {
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Mutex initialization failed\n");
        return -1;
    }
    if (pthread_cond_init(&cond_que, NULL) != 0) {
        printf("cond_que initialization failed\n");
        pthread_mutex_destroy(&mutex);
        return -2;
    }
    return 0;
}

//destroying mutex and condition variable
void destroy_sync_primitives() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_que);
}

//splitting the arguments so they ready for execvp to execute
void split_to_arg(char *param, char **vp_arguments) {
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
    *vp_arguments = NULL;
}

void *controller_thread(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);
    char job[max_command_size], buffer[max_command_size], *comm, jobID[12], *parameter;

    pthread_mutex_lock(&mutex); //locking mutex so the operations are being done smoothly without interference of other threads
    int bytes_received = read(client_socket, job, max_command_size);

    if (bytes_received < 0) {
        close(client_socket);
        pthread_mutex_unlock(&mutex);
        return NULL;
    } else if (bytes_received >= 0) {
        buffer[0] = '\0';
        jobID[0] = '\0';
        job[bytes_received] = '\0';
        fflush(stdout);
        comm = strtok(job, " ");
        if (strcmp(comm, "issueJob") == 0) {
            parameter = strtok(NULL, "");
            sprintf(jobID, "job_%d", job_ID_int);
            job_ID_int++;
            if (que_Enqueue(jobID, parameter, client_socket, &QD) == 0) {
                job_ID_int--;
                pthread_mutex_unlock(&mutex);
                pthread_cond_broadcast(&cond_que);
                return NULL;
            }
            sprintf(buffer, "JOB <%s, %s> SUBMITTED\n", jobID, parameter);
            write(client_socket, buffer, strlen(buffer));
        } else if (strcmp(comm, "setConcurrency") == 0) {
            parameter = strtok(NULL, " ");
            if(atoi(parameter) <= threadPoolSize) {
                concurrency = atoi(parameter);
                sprintf(buffer, "CONCURRENCY SET AT %d\n", concurrency);
                write(client_socket, buffer, strlen(buffer));
                write(client_socket, "close", 5);

                pthread_cond_broadcast(&cond_que); // "notyfying" workers for new concurrency
            }
            else { //making sure the concurrency given is not greater than the limit we gave to the server
                sprintf(buffer, "CONCURRENCY CAN'T BE SET TO BE MORE THAN THE THREADPOOL SIZE, SO TRY AGAIN!\n");
                write(client_socket, buffer, strlen(buffer));
                write(client_socket, "close", 5);
            }
        } else if (strcmp(comm, "stop") == 0) {
            parameter = strtok(NULL, "");
            if (que_stop_jobID(parameter, &QD) == 1) {
                sprintf(buffer, "JOB <%s> REMOVED\n", parameter);
                write(client_socket, buffer, strlen(buffer));
                write(client_socket, "close", 5);
            } else {
                sprintf(buffer, "JOB <%s> NOT FOUND\n", parameter);
                write(client_socket, buffer, strlen(buffer));
                write(client_socket, "close", 5);
            }
        } else if (strcmp(comm, "poll") == 0) {
            que_print(buffer, &QD);
            write(client_socket, buffer, strlen(buffer));
            write(client_socket, "close", 5);
        } else if (strcmp(comm, "exit") == 0) {
            sprintf(buffer, "SERVER TERMINATED\n"); //sending this to the commander that sent exit
            write(client_socket, buffer, strlen(buffer));
            write(client_socket, "close", 5);

            int socket;
            if(que_SizeOf(&QD) > 0) {
                socket = (&QD)->Front->socket_num;
                strcpy(jobID, (&QD)->Front->jobID);
            }
            while (que_SizeOf(&QD) > 0) {
                que_Pop(&QD);
                sprintf(job, "SERVER TERMINATED BEFORE EXECUTION\n"); //sending this to the commanders of the issuejobs in the queue if there are any
                write(socket, job, strlen(job));
                write(socket, "close", 5);
                if(que_SizeOf(&QD) > 0) {
                    socket = (&QD)->Front->socket_num;
                    strcpy(jobID, (&QD)->Front->jobID);
                }
            }

            terminate = 1;
            pthread_mutex_unlock(&mutex);
            pthread_cond_broadcast(&cond_que); // Wake up all worker threads

            shutdown(server_socket, SHUT_RDWR); // Shutting down server socket to unblock accept
            close(server_socket);
            return NULL;
        }
    }
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond_que);
    return NULL;
}

void *worker_thread(void *arg) {
    pid_t id;
    char *exec_file, job_id[max_command_size], job[max_command_size], save_job[max_command_size], save_job_2[max_command_size];
    int client_socket;

    while (!terminate) {
        job_id[0] = '\0';
        job[0] = '\0';
        char *vp_arguments[100];

        pthread_mutex_lock(&mutex);
        while ((que_SizeOf(&QD) == 0 || active_workers >= concurrency) && !terminate) {
            pthread_cond_wait(&cond_que, &mutex);
        }

        if (terminate && que_SizeOf(&QD) >= 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        strcpy(job_id, (&QD)->Front->jobID);
        strcpy(job, (&QD)->Front->job);
        client_socket = (&QD)->Front->socket_num;

        strcpy(save_job, job);
        exec_file = strtok(job, " ");
        char exec_str[max_command_size];
        if (access(exec_file, X_OK) == 0) { //checking if it is executable or not
            sprintf(exec_str, "./%s", exec_file);
        } else {
            sprintf(exec_str, "%s", exec_file);
        }
        strcpy(save_job_2, save_job);
        split_to_arg(save_job_2, vp_arguments);

        que_Pop(&QD);
        active_workers++; //counting the workers that are axtually running jobs
        pthread_mutex_unlock(&mutex);

        id = fork();
        if (id == -1) {
            printf("\nfork didn't work!\n");
        } else if (id == 0) { //child proccess
            char output_filename[50];
            sprintf(output_filename, "%d.output", getpid());
            int fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open");
                exit(EXIT_FAILURE);
            }

            if (dup2(fd, STDOUT_FILENO) == -1) { //redirecting stdout so the output gets "printed" in the file we created
                perror("dup2");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);

            execvp(exec_str, vp_arguments); //executing job
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { //parent proccess
            size_t bytesRead;
            char buffer[max_command_size];
            char output[max_command_size];
            output[0] = '\0';

            waitpid(id, NULL, 0); //waiting for child to finish executing the job
            char output_filename[50];
            sprintf(output_filename, "%d.output", id);

            FILE *fp = fopen(output_filename, "r");
            if (fp == NULL) {
                perror("fopen");
                pthread_mutex_lock(&mutex);
                active_workers--;
                pthread_mutex_unlock(&mutex);
                continue;
            }

            while ((bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp)) > 0) {
                buffer[bytesRead] = '\0';
                strncat(output, buffer, bytesRead);
            }
            fclose(fp);

            char final_output[max_command_size]; //cunstructing the final string that will be sent back to commander
            sprintf(final_output,"----- %s output start------\n%s\n----- %s output end------\n", job_id, output, job_id);
            final_output[max_command_size - 1] = '\0';

            write(client_socket, final_output, strlen(final_output));
            write(client_socket, "close", 5);
            remove(output_filename);
        }

        pthread_mutex_lock(&mutex);
        active_workers--; //worker finished executing so we letting other workers know
        pthread_cond_broadcast(&cond_que);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s [portnum] [bufferSize] [threadPoolSize]\n", argv[0]);
        return 1;
    }
    initialize();

    main_thread_id = pthread_self();

    int portnum = atoi(argv[1]);
    int buf_Size_main = atoi(argv[2]);
    threadPoolSize = atoi(argv[3]);

    buffer_size = buf_Size_main;

    que_Create(&QD);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portnum);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_socket);
        return 1;
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, REQUESTS) < 0) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    printf("Server listening on port %d\n", portnum);

    pthread_t thread_pool[threadPoolSize];
    for (int i = 0; i < threadPoolSize; ++i) {
        pthread_create(&thread_pool[i], NULL, &worker_thread, NULL);
    }

    while (!terminate) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int *client_socket = malloc(sizeof(int));
        if (client_socket == NULL) {
            printf("Error malloc----\n");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        *client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket < 0) {
            free(client_socket);
            continue;
        }

        pthread_t det_controller_thread;
        if (pthread_create(&det_controller_thread, NULL, controller_thread, client_socket) != 0) {
            perror("pthread_create");
            free(client_socket);
            close(*client_socket);
            continue;
        }
        pthread_detach(det_controller_thread);
    }

    // Waiting for worker threads to finish
    for (int i = 0; i < threadPoolSize; ++i) {
        pthread_join(thread_pool[i], NULL);
    }

    destroy_sync_primitives();
    return 0;
}
