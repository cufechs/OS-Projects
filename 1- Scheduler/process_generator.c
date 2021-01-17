#include "headers.h"

struct Process** Processes; //Declared global to clean them upon interruption

int NumberOfProcesses;


int shmid_PG1;
struct Process** shmadr_PG1;
int shmid_PG2;
pid_t* shmadr_PG2;
int semid_PG1;

union Semun semun;



void clearResources(int);
void createAttachResources();


int main(int argc, char * argv[])
{
    signal(SIGINT, clearResources);
    // TODO Initialization
    // 1. Read the input files.
    
    FILE * pFile; //Openning the procceses file for reading
    pFile = fopen("processes.txt", "r");
    
    NumberOfProcesses = -1; //Equal to -1 to skip the first line "Comment"
    
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
    
    	Processes = malloc(sizeof(struct Process*) * NumberOfProcesses); //allocating memory for processes
    	for(int i=0; i<NumberOfProcesses; i++) //Filling the processes
    	{
    		Processes[i] = malloc(sizeof(struct Process));
			fscanf(pFile, "%s", Word); //Read string at a time
			Processes[i]->ID = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Arrival = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Runtime = atoi(Word);
			fscanf(pFile, "%s", Word);
			Processes[i]->Priority = atoi(Word);
			Processes[i]->RemainingTime = Processes[i]->Runtime;
      	}
    }
    else
    {
    	printf("procceses file doesn't exist!");
    	return 0;
    }
    
    fclose(pFile);
    
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    
    int ChosenSchedulingAlgorithm = 0, Quantum = 1;
    
    printf("Scheduling Algorithm:\nFor Round Robin Enter 0\nFor Highest Priority First Enter 1\nFor Shortest Remaining Time Enter 2\n");
    
    scanf("%d", &ChosenSchedulingAlgorithm);
    
    if(ChosenSchedulingAlgorithm == 0) //Round Robin
    {
		printf("Enter the quantum value for Round Robin: ");
		
		scanf("%d", &Quantum);
    }
    
	createAttachResources();
    
    if(fork() == 0){  //Forking the clock to start it and initalize it, without pausing this program
    	
		char char_arg [100];
		if(ChosenSchedulingAlgorithm == 0)
			sprintf(char_arg, "./scheduler.out %d %d",ChosenSchedulingAlgorithm, Quantum);
		else
    		sprintf(char_arg, "./scheduler.out %d",ChosenSchedulingAlgorithm);
		int Status = system(char_arg);
        exit(0);
    }
   
    if(fork() == 0){  //Forking the clock to start it and initalize it, without pausing this program
        int Status = system("./clk.out");
        exit(0);
    }
    
    
    initClk();    
    int my_clk;
    
	
    shmadr_PG1 = NULL; /* initialize shared memory */
	down(semid_PG1); // to recive scheduler pid
	int PID_SCHD = *shmadr_PG2;
	
	printf("OK2, pid: %d\n", PID_SCHD);

    
    while(1){
    	my_clk = getClk();
    	
    	for(int i=0; i<NumberOfProcesses; i++)
    	{
			if(Processes[i]->Arrival == my_clk){
				printf("Hi from PG\n");
				
				shmadr_PG1 = &Processes[i];
				printf("-%p- PG send: id: %d, arr: %d, runtime: %d, p: %d.\n",
				shmadr_PG1,(*shmadr_PG1)->ID, (*shmadr_PG1)->Arrival, (*shmadr_PG1)->Runtime, (*shmadr_PG1)->Priority);
				kill(PID_SCHD, SIGUSR1);

				NumberOfProcesses--;
				Processes[i] = Processes[NumberOfProcesses]; //Decreasing the number of processes
				
				down(semid_PG1);
			}
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
    //We Should free the processes
    for(int i=0; i<NumberOfProcesses; i++)
    	free(Processes[i]);
	free(Processes);
	destroyClk(true);
}

void createAttachResources(){

	shmid_PG1 = shmget(321231, sizeof(struct Process*), IPC_CREAT | 0666);
    if ((long)shmid_PG1 == -1){
        perror("Error in creating shm! in process generator!");
        exit(-1);
    }
    shmadr_PG1 = (struct Process**) shmat(shmid_PG1, (void *)0, 0);
    if ((long)shmadr_PG1 == -1){
        perror("Error in attaching the shm in process generator!");
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

	semid_PG1 = semget(3223234, 1, 0666 | IPC_CREAT);
	if ((long)shmadr_PG1 == -1){
        perror("Error in creating sem! in process generator!");
        exit(-1);
    }
	union Semun semun;
	semun.val = 0; /* initial value of the semaphore, Binary semaphore */
    if (semctl(semid_PG1, 0, SETVAL, semun) == -1){
        perror("Error in semaphore");
        exit(-1);
    }

	//shmadr_PG1 = (struct Process*) malloc(sizeof(struct Process));
}