
typedef struct que_DequeNodeTag *que_Dequepointer;

typedef struct que_DequeNodeTag {
    int quequePositon;
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
void que_Enqueue(char *jobID,char *job, queued_Deque *); //inserts an item in the end of the queue
void que_Pop(queued_Deque *D); //removes an item from the start of the queue 
int que_stop_jobID(char *,queued_Deque *); //removes stops the job_id we give it from executing
void que_print(char *,queued_Deque *); //prints all the jobs that are running


typedef struct run_DequeNodeTag {
    pid_t pid;
    int quequePositon;
    char jobID[100];
    char job[2048];
    struct run_DequeNodeTag *Link;
} running_DequeNode;

typedef struct run_DequeTag {
    running_DequeNode *Front;
    running_DequeNode *Rear;
    int number_of_items;
} running_Deque;

typedef struct run_DequeNodeTag *run_Dequepointer;
void run_Create(running_Deque *); //Creates an empty Deque
int run_SizeOf(running_Deque *); //rerutns the number of items of that are in the Deque
void run_Enqueue(pid_t pid,char *jobID,char *job, running_Deque *D); //inserts an item in the end of the queue
void run_Pop(char *jobID,char * job, running_Deque *D); //removes an item from the start of the queue 
void run_eliminating_pid(pid_t id,running_Deque *D); //removes stops the proccess_id node it gets from the running queue
int run_stop_jobID(char *jobID,running_Deque *D); //removes stops the job_id we give it from executing
void run_print(char *,running_Deque *); //prints all the jobs that are running

