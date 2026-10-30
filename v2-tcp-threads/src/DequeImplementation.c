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
#include "DequeInterface.h"
#include "variables.h"


void que_Create(queued_Deque *D) {
    D->Front=NULL;
    D->Rear=NULL;
    D->number_of_items = 0;
}

int que_SizeOf(queued_Deque* D)
{
    return D->number_of_items;
}

int que_Enqueue(char *jobID,char *job,int socket, queued_Deque *D) {
    if (D->number_of_items >= buffer_size) {
        // printf("Queue is full. Cannot enqueue new item.\n");
        write(socket, "Queue is full. Cannot enqueue new item.", 40);
        write(socket, "close", 5);
        return 0;
    }

    queued_DequeNode *Temp;
    Temp = (queued_DequeNode *)malloc(sizeof(queued_DequeNode)); //allocates memory for the new node
    if (Temp==NULL) {
        printf("the memory is full\n");
        return 0;
    }
    else {
        Temp->Link=NULL;
        strcpy(Temp->job,job);
        strcpy(Temp->jobID,jobID);
        Temp->socket_num = socket;
        if(D->Rear==NULL) { //if the Deque is previously empty it sets the front and last item of the Deque as the first one  
            D->Front=Temp; 
            D->Rear=Temp;
        }
        else {
            D->Rear->Link=Temp;
            D->Rear=Temp;
        }
        (D->number_of_items)++; //adds 1 to the numbers of items that are in the Deque
        Temp->quequePositon = D->number_of_items;
    }
    return 1;
}


int que_Pop(queued_Deque *D) { //returns 0 if there is not item on 
    queued_DequeNode *Temp;
    if(D->Front == NULL) {
        // printf("\nattempting to pop an empty deque(que_Pop)\n");
        return 0;
    }
    else {
        Temp=D->Front;
        D->Front=Temp->Link;
        free(Temp);
        if (D->Front == NULL) D->Rear = NULL; //if the last item was deleted it sets the pointer to the rear item of the deque to null
        (D->number_of_items)--; //reduces the number of items by 1 
    }
    if(D->Front != NULL) {
        queued_DequeNode *Temp1;
        Temp1 = D->Front;
        Temp1->quequePositon = 1;
        int i = 2;
        while(Temp1->Link!=NULL) {
            Temp1 = Temp1->Link;
            Temp1->quequePositon = i;
            i++;
        }
    }
    return 1;
}

int que_stop_jobID(char *jobID,queued_Deque *D) {
    if(D->Front==NULL) {
        return 0;
    }

    queued_DequeNode *curr = D->Front;
    queued_DequeNode *prev = NULL;

    while (curr != NULL) {
        if (strcmp(curr->jobID, jobID) == 0) {
            if (prev == NULL) {
                D->Front = curr->Link;
                write(curr->socket_num, "close", 5);
                free(curr);
                if (D->Front == NULL) D->Rear = NULL;
            } else {
                prev->Link = curr->Link;
                write(curr->socket_num, "close", 5);
                free(curr);
                if (prev->Link == NULL) D->Rear = prev;
            }
            D->number_of_items--;
            if(D->Front != NULL) {
                queued_DequeNode *Temp1;
                Temp1 = D->Front;
                Temp1->quequePositon = 1;
                int i = 2;
                while(Temp1->Link!=NULL) {
                    Temp1 = Temp1->Link;
                    Temp1->quequePositon = i;
                    i++;
                }
            }
            return 1; // stopped successfully
        }
        prev = curr;
        curr = curr->Link;
    }
    return 0;
}

void que_print(char *str_to_comm,queued_Deque *D) {
    queued_DequeNode *Temp;
    if(D->Front == NULL) {
        sprintf(str_to_comm,"There are no queued jobs !\n");
        return;
    }
    Temp = D->Front;
    while (Temp != NULL)
    {
        sprintf(str_to_comm,"%s<%s, %s>\n",str_to_comm,Temp->jobID,Temp->job);
        Temp = Temp->Link;
    }
}
