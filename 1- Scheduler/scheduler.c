#include "headers.h"

struct Node* ReadyQueue = NULL; //This is the start of the -Not Official- Linked List
struct Process* runningProcess = NULL;
int SchedulingAlgorithm;


int shmid_PG1;
struct Process** shmadr_PG1;
int shmid_PG2;
pid_t* shmadr_PG2;
int shmid_SCH1;
pid_t* shmadr_SCH1;
int semid_PG1;
int semid_SHC1;

void CleanUp(int signum);
void createAttachResources();
void ProcessArrived(int signum);
void removeFromReadyQueue(struct Node* node);



int main(int argc, char * argv[])
{
	signal(SIGUSR1, ProcessArrived);
	signal(SIGINT, CleanUp);
	
	createAttachResources();

	*shmadr_PG2 = getpid();
	printf("OK1, pid: %d\n", *shmadr_PG2);
	up(semid_PG1);


    initClk();
    

    SchedulingAlgorithm = atoi(argv[1]); //Should be sent from outside
	int Quantum = (SchedulingAlgorithm==0)? atoi(argv[2]) : 1; // get the quantum if RR
	int PrevClk = getClk() - 1;
	switch(SchedulingAlgorithm)
	{
		case 0: //RR
			while(1)
			{
				if(getClk() - PrevClk > 1)
				{
					
					
					PrevClk = getClk();
				}
			}
			break;
		case 1: //HPF
			while(1){

				//if(getClk() - PrevClk > 1){
					if(ReadyQueue != NULL) {

						struct Node* BestPriority = ReadyQueue;
						struct Node* Temp = ReadyQueue;
						
						while(Temp != NULL)
						{
							if(Temp->Value->Priority < BestPriority->Value->Priority)
								BestPriority = Temp;
								
							Temp = Temp->Next;	
						}

						runningProcess = BestPriority->Value;
						removeFromReadyQueue(BestPriority);
						
						
						if(fork() == 0){
							char char_arg[100]; 
							sprintf(char_arg, "./process.out %d %d", BestPriority->Value->Runtime, getppid());
							int Status = system(char_arg);
							exit(0);
						}

						down(semid_SHC1);
						int PID = *shmadr_SCH1;
						printf("OK2, pid: %d\n", PID);
						
						int stat_loc;
						waitpid(PID, &stat_loc, 0);
						if(WIFEXITED(stat_loc)){}
						
						printf(":)\n");
						//Archive
						
					}
					
					PrevClk = getClk();
				//}
			}
			break;
		case 2: //SRTN
			while(1)
			{
				if(getClk() - PrevClk > 1)
				{
					
					
					PrevClk = getClk();
				}
			}
			break;
	}
    
    //upon termination release the clock resources.
    
    CleanUp(0);
}

void ProcessArrived(int signum) //Process generator signals the scheduler that there is a process arrived and should be taken from the shared memory
{
	struct Node* NewNode = (struct Node*)malloc(sizeof(struct Node));
	NewNode->Next = NULL;
	NewNode->Value = (struct Process*)malloc(sizeof(struct Process));
	*NewNode->Value = **(struct Process**)shmadr_PG1;

	printf("-- Sch Rec: id: %d, arr: %d, runtime: %d, p: %d.\n",
	 NewNode->Value->ID, NewNode->Value->Arrival, NewNode->Value->Runtime, NewNode->Value->Priority);

	if(ReadyQueue == NULL){
		ReadyQueue = NewNode;
		up(semid_PG1);
		return;
	}
	
	struct Node* Temp = ReadyQueue;
	
	while(Temp->Next != NULL){
		Temp = Temp->Next;
	}

	Temp->Next = NewNode;

	up(semid_PG1);
}

void removeFromReadyQueue(struct Node* node){

	struct Node* temp = ReadyQueue;

	if(node == ReadyQueue){
		ReadyQueue == NULL;
		node->Next = NULL;
		return;
	}

	while(temp->Next != NULL){
		if(node == temp->Next){
			temp->Next = node->Next;
			node->Next = NULL;
			return;
		}
	}
}

/* Clear the resources before exit */
void CleanUp(int signum)
{
    shmctl(shmid_PG1, IPC_RMID, NULL);
    destroyClk(true);
    printf("Scheduler terminating!\n");
    exit(0);
}

void createAttachResources(){

	shmid_PG1 = shmget(321231, sizeof(struct Process*), 0666);
    if ((long)shmid_PG1 == -1){
        perror("Error in creating shm! in Scheduler!");
        exit(-1);
    }
    shmadr_PG1 = (struct Process**) shmat(shmid_PG1, (void *)0, 0);
    if ((long)shmadr_PG1 == -1){
        perror("Error in attaching the shm in Scheduler!");
        exit(-1);
    }

	shmid_PG2 = shmget(1279456, sizeof(pid_t), IPC_CREAT | 0666);
    if ((long)shmid_PG2 == -1){
        perror("Error in creating shm! in process generator!");
        exit(-1);
    }
    shmadr_PG2 = (pid_t*) shmat(shmid_PG2, (void *)0, 0);
    if ((long)shmadr_PG2 == -1){
        perror("Error in attaching the shm in process generator!");
        exit(-1);
    }

	shmid_SCH1 = shmget(53242, sizeof(pid_t), IPC_CREAT | 0666);
    if ((long)shmid_SCH1 == -1){
        perror("Error in creating shm! in process generator!");
        exit(-1);
    }
    shmadr_SCH1 = (pid_t*) shmat(shmid_SCH1, (void *)0, 0);
    if ((long)shmadr_SCH1 == -1){
        perror("Error in attaching the shm in process generator!");
        exit(-1);
    }

	semid_PG1 = semget(3223234, 1, 0666);
	if ((long)shmadr_PG1 == -1){
        perror("Error in creating sem! in Scheduler!");
        exit(-1);
    }

	semid_SHC1 = semget(512432, 1, IPC_CREAT | 0666);
	if ((long)semid_SHC1 == -1){
        perror("Error in creating sem! in Scheduler!");
        exit(-1);
    }
	union Semun semun;
	semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(semid_SHC1, 0, SETVAL, semun) == -1){
        perror("Error in semaphore");
        exit(-1);
    }
}
