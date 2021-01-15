#include "headers.h"

int shmid;
Node* ReadyQueue = nullptr; //This is the start of the -Not Official- Linked List
struct Process*** shmaddr;
int SchedulingAlgorithm;

struct Process
{
	int ID;
	int Arrival;
	int Runtime;
	int Priority;
	int RemainingTime;
};

struct Node
{
	Node* Next;
	struct Process* Value;
};

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
	Node*Temp = ReadyQueue;
	
	int OldCount = 0;
	while(Temp != nullptr)
	{
		Temp = Temp->Next;
		OldCount++;
	}
		
	struct Process** Processes = *shmaddr;
	
	int Counter = 0;
	while(Processes[Counter] != nullptr)
	{
		Node* NewNode = (Node*)malloc(sizeof(Node));
		NewNode->Next = nullptr;
		NewNode->Value = *Processes[Counter];
		*Temp = *NewNode;
		Temp = *(Temp).Next;
		
		Counter++;
	}
	
	if(SchedulingAlgorithm == 1) //HPF sorting
	{
		Node* Temp2 = ReadyQueue;
		Node* Temp3 = ReadyQueue;
		for(int i=0; i<(OldCount+Counter)-1; i++)
		{
			for(int j=i+1; j<(OldCount+Counter); j++)
			{
				if(Temp2->Value->Priority > Temp3->Value->Priority)
				{
					struct Process* Temp4 = Temp2->Value;
					*(Temp2) = *(Temp3); //We Stopped Here
				}
			}
		}
	}
	else if(SchedulingAlgorithm == 2) //SRTN sorting
	{
	
	}
}

int main(int argc, char * argv[])
{
	signal(SIGUSR1, ProcessArrived);
	signal(SIGINT, CleanUp);
	
	ReadyQueue = (Node*)malloc(sizeof(Node));
	
	//This shared memory is used for passing processes to the scheduler on arrival time
	//Creating shared memory
	shmid = shmget(SHKEYPROCESS, 4096, 0644);
    while ((long)shmid == -1)
        shmid = shmget(SHKEYPROCESS, 4096, 0644);
    
    shmaddr = (struct Process***) shmat(shmid, (void *)0, 0);
    if ((long)shmaddr == -1)
    {
        perror("Error in attaching the shm in clock!");
        exit(-1);
    }

    initClk();
    
    //TODO implement the scheduler :)
    SchedulingAlgorithm = atoi(argv[1]); //Should be sent from outside
	switch(SchedulingAlgorithm)
	{
		case 0: //RR
			int Quantum = atoi(argv[2]);
			while(1)
			{
				
			}
			break;
		case 1: //HPF
			while(1)
			{
				
			}
			break;
		case 2: //SRTN
			while(1)
			{
				
			}
			break;
	}
    
    //upon termination release the clock resources.
    
    CleanUp(0);
}
