typedef struct que_DequeNodeTag *que_Dequepointer;

typedef struct que_DequeNodeTag {
    int quequePositon;
    int socket_num;
    char jobID[100];
    char job[2048];
    struct que_DequeNodeTag *Link;
} queued_DequeNode;

typedef struct que_DequeTag {
    queued_DequeNode *Front;
    queued_DequeNode *Rear;
    int number_of_items;
} queued_Deque;


void que_Create(queued_Deque *); //Creates an empty Deque
int que_SizeOf(queued_Deque *); //rerutns the number of items of that are in the Deque
int que_Enqueue(char *jobID,char *job,int socket, queued_Deque *D); //inserts an item in the end of the queue
int que_Pop(queued_Deque *D); //removes an item from the start of the queue 
int que_stop_jobID(char *,queued_Deque *); //removes stops the job_id we give it from executing
void que_print(char *,queued_Deque *); //prints all the jobs that are running