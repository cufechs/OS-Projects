#include "headers.h"

int shmid;
struct Node* ReadyQueue = NULL; //This is the start of the -Not Official- Linked List
struct Process*** ProcSch_shmaddr;
int SchedulingAlgorithm;


/* Clear the resources before exit */
void CleanUp(int signum)
{
    shmctl(shmid, IPC_RMID, NULL);
    destroyClk(true);
    printf("Scheduler terminating!\n");
    exit(0);
}

void ProcessArrived(int signum) //Process generator signals the scheduler that there is a process arrived and should be taken from the shared memory
{
	struct Node* Temp = ReadyQueue;
	
	int OldCount = 0;
	while(Temp != NULL)
	{
		Temp = Temp->Next;
		OldCount++;
	}
	
	if(Temp == NULL)
		ReadyQueue = (struct Node*)malloc(sizeof(struct Node));
		
	struct Process** Processes = *ProcSch_shmaddr;
	
	int Counter = 0;
	while(Processes[Counter] != NULL)
	{
		struct Node* NewNode = (struct Node*)malloc(sizeof(struct Node));
		NewNode->Next = NULL;
		NewNode->Value = (struct Process*)malloc(sizeof(struct Process));
		*(NewNode->Value) = *(Processes[Counter]);
		
		Temp = NewNode;
		Temp = Temp->Next;
		
		Counter++;
	}
}

int main(int argc, char * argv[])
{
	signal(SIGUSR1, ProcessArrived);
	signal(SIGINT, CleanUp);
	
	//This shared memory is used for passing processes to the scheduler on arrival time
	//Creating shared memory
	shmid = shmget(SHKEYPROCESS, 4096, 0644);
    while ((long)shmid == -1)
        shmid = shmget(SHKEYPROCESS, 4096, 0644);
    
    ProcSch_shmaddr = (struct Process***) shmat(shmid, (void *)0, 0);
    if ((long)ProcSch_shmaddr == -1)
    {
        perror("Error in attaching the shm in clock!");
        exit(-1);
    }

    initClk();
    
    //TODO implement the scheduler :)
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
			while(1)
			{
				if(getClk() - PrevClk > 1)
				{
					if(ReadyQueue != NULL)
					{
						struct Node* BestPriority = ReadyQueue;
						struct Node* Temp = ReadyQueue;
						
						while(Temp != NULL)
						{
							if(Temp->Value->Priority < BestPriority->Value->Priority)
								BestPriority = Temp;
								
							Temp = Temp->Next;	
						}
						
						int PID = fork();
						if(PID == 0)
						{
							char char_arg[100]; 
							sprintf(char_arg, "./process.out %d %d", BestPriority->Value->Runtime, getppid());
							int Status = system(char_arg);
							exit(0);
						}
						
						int stat_loc;
						waitpid(PID, &stat_loc, 0);
						if(WIFEXITED(stat_loc)){}
						
						//Archive
						
					}
					
					PrevClk = getClk();
				}
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
