#include "headers.h"

struct Process
{
	int ID;
	int Arrival;
	int Runtime;
	int Priority;
	int RemainingTime;
};

struct Process** Processes; //Declared global to clean them upon interruption
int shmid;
struct Process*** ProcSch_shmaddr;

void clearResources(int);

int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    
    FILE * pFile; //Openning the procceses file for reading
    pFile = fopen("processes.txt", "r");
    
    int NumberOfProcesses = -1; //Equal to -1 to skip the first line "Comment"
    
    if(pFile != 0) //File exists
    {
    	while(1)  //Identify the number of processes
    	{
    		char Word[20];
			
			fgets(Word, 1000, pFile);
			
			if(feof(pFile)) //End of file!
				break;
				
			NumberOfProcesses++;
      	}
      	
      	rewind(pFile); //Go to beginning of the file
      	
      	if(NumberOfProcesses <= 0)
      	{
      		printf("No Processes found!");
      		return 0;
      	}
      	
      	char Word[20];
      	fgets(Word, 1000, pFile);
    
    	Processes = malloc(sizeof(int) * 5 * NumberOfProcesses); //allocating memory for processes
    	for(int i=0; i<NumberOfProcesses; i++) //Filling the processes
    	{
			fscanf(pFile, "%s", Word); //Read string at a time
			Processes[i]->ID = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Arrival = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Runtime = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Priority = atoi(Word);
			Processes[i]->RemainingTime = Processes[i].Runtime;
      	}
    }
    else
    {
    	printf("procceses file doesn't exist!");
    	return 0;
    }
    
    fclose(pFile);
    
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    
    int ChosenSchedulingAlgorithm = 0, Quantum = 0;
    
    printf("Scheduling Algorithm:\nFor Round Robin Enter 0\nFor Highest Priority First Enter 1\nFor Shortest Remaining Time Enter 2\n");
    
    scanf("%d", &ChosenSchedulingAlgorithm);
    
    if(ChosenSchedulingAlgorithm == 0) //Round Robin
    {
		printf("Enter the quantum value for Round Robin: ");
		
		scanf("%d", &Quantum);
    }
    
    // 3. Initiate and create the scheduler and clock processes.
    
    //Creating Scheduler here!
    int PID_SCHD = fork();
    if(PID_SCHD == 0){  //Forking the clock to start it and initalize it, without pausing this program
    	if(ChosenSchedulingAlgorithm == 0)
        	int Status = system("./scheduler.out %d %d", ChosenSchedulingAlgorithm, Qunatum);
    	else
    		int Status = system("./scheduler.out %d", ChosenSchedulingAlgorithm);
        exit(0);
    }
    
   
    if(fork() == 0){  //Forking the clock to start it and initalize it, without pausing this program
        int Status = system("./clk.out");
        exit(0);
    }
    
    // 4. Use this function after creating the clock process to initialize clock
    
    initClk();
    
    // To get time use this
    
    int x = getClk();
    printf("current time is %d\n", x);
    
    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    
    //This is already done above!
    
    // 6. Send the information to the scheduler at the appropriate time.
    
    //This shared memory is used for passing processes to the scheduler on arrival time
	//Creating shared memory
	shmid = shmget(SHKEYPROCESS, 4096, IPC_CREAT | 0644);
    if ((long)shmid == -1)
    {
        perror("Error in creating shm!");
        exit(-1);
    }
    
    ProcSch_shmaddr = (struct Process*) shmat(shmid, (void *)0, 0);
    if ((long)ProcSch_shmaddr == -1)
    {
        perror("Error in attaching the shm in process generator!");
        exit(-1);
    }
    *ProcSch_shmaddr = nullptr; /* initialize shared memory */
    
    while(1)
    {
    	x = getClk();
    	
    	int Counter = 0;
    	for(int i=0; i<NumberOfProcesses; i++)
			if(Processes[i]->Arrival == x)
				Counter++;
    	
    	struct Process** Temp = (struct Process**)malloc(sizeof(struct Process*)*(Counter+1));
    	
    	Counter = 0;
    	for(int i=0; i<NumberOfProcesses; i++)
    	{
			if(Processes[i]->Arrival == x)
			{
				/*
				Here, scheduler takes the process at the appropriate arrival time //TODO
				*/
				printf("current time is %d\n", x);
				
				Temp[Counter++] = Processes[i];
				NumberOfProcesses--;
				Processes[i] = Processes[NumberOfProcesses]; //Decreasing the number of processes
			}
    	}
    	
    	//How will the schdeuler know how man processes are there? there you go
    	if(Counter > 0)
    	{
			Temp[Counter] = nullptr;
			*ProcSch_shmaddr = Temp;
			kill(PID_SCHD, SIGUSR1);
			//Note: we will not need to make a semaphore here, as there is no situation that there is two processes will access the shared memory at the same time.
    	}
    	
    	if(NumberOfProcesses == 0)
    		break;
    }
    
    // 7. Clear clock resources
    clearResources(0);
    
    return 0;
}

void clearResources(int signum)
{
    //TODO Clears all resources in case of interruption
	free(Processes);
	destroyClk(true);
}
