#include "headers.h"

struct ArchiveNode* ArchiveHead = NULL;
struct ArchiveNode* ArchiveHead2 = NULL;
struct Node* archive = NULL;
struct Node* archive_tail = NULL;

struct Node* ReadyQueue = NULL; //This is the start of the -Not Official- Linked List
struct Process* runningProcess = NULL;
int SchedulingAlgorithm;
bool preemption_flag = false;
bool processFinished_flag = false;


int shmid_PG1;
struct Process* shmadr_PG1;
int shmid_PG2;
pid_t* shmadr_PG2;
int shmid_SCH1;
pid_t* shmadr_SCH1;
int semid_PG1;
int semid_PG2;
int semid_SHC1;
int semid_SHC2;
int semid_CLK;
int semid_PG_CLK;

void CleanUp(int signum);
void createAttachResources();
void ProcessArrived(int signum);
void ProcessFinished(int signum);
void removeFromReadyQueue(struct Node* node);
void addToReadyQueue(struct Node* node);
void addToArchive(struct Process* proc);



int main(int argc, char * argv[])
{
	signal(SIGUSR1, ProcessArrived);
	signal(SIGUSR2, ProcessFinished);
	signal(SIGINT, CleanUp);

	createAttachResources();

	*shmadr_PG2 = getpid();
	//printf("OK1, pid: %d\n", *shmadr_PG2);
	up(semid_PG1);
    
    initClk();
	initMemMngr();
	up(semid_PG1);
    

    SchedulingAlgorithm = atoi(argv[1]); //Should be sent from outside
	int Quantum = (SchedulingAlgorithm==RR)? atoi(argv[2]) : 1; // get the quantum if RR
	int currQuantum;
	int PrevClk = getClk() - 1;
	switch(SchedulingAlgorithm)
	{
		case RR:
			while(1){
				if(runningProcess == NULL){

					if(ReadyQueue != NULL) {
						
						struct Node* temp = ReadyQueue;
                            printf("Ready Queue: ");
                            while(temp != NULL){
                                printf("%d ", temp->Value->ID);
                                temp = temp->Next;
                            }
                            printf("\n");

						
						if(ReadyQueue->Value->generated == false){
							

							if(fork() == 0){
								char char_arg[100]; 
								sprintf(char_arg, "./process.out %d %d %d", ReadyQueue->Value->Runtime, getppid(), getClk());
								int Status = system(char_arg);
								exit(0);
							}
							
							//Might need a semaphore
							//
							//struct memStruct alloc = allocateProcess(ReadyQueue->Value->memSize, ReadyQueue->Value->ID);
							int Clk = getClk();
							if(ArchiveHead == NULL)
							{
								ArchiveHead = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime;
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ArcInstance.RunTime;
								ArcInstance.EventTime = Clk;
								ArcInstance.State = 0;
								
								ArchiveHead->ArchiveInstance = ArcInstance;
								ArchiveHead->Next = NULL;
							}
							else
							{
								
								struct ArchiveNode* Temp = ArchiveHead;
								while(Temp->Next != NULL)
									Temp = Temp->Next;
									
								Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime;
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ArcInstance.RunTime;
								ArcInstance.EventTime = Clk;
								ArcInstance.State = 0;
								
								Temp->Next->ArchiveInstance = ArcInstance;
								Temp->Next->Next = NULL;
							}
							//
							
							usleep(100);
							down(semid_SHC2);
							down(semid_PG2);
							down(semid_SHC1);
							up(semid_PG2);
							up(semid_SHC2);
							int PID = *shmadr_SCH1;
							//printf("OK2, pid: %d\n", PID);
							ReadyQueue->Value->pid = PID;
							ReadyQueue->Value->RemainingTime = ReadyQueue->Value->Runtime;
							printf("clk: %d  \t ID: %d will start\n",getClk(), ReadyQueue->Value->ID);	
						}
						else{
							//Might need a semaphore
							//
							int Clk = getClk();
							struct ArchiveNode* Temp = ArchiveHead;
							while(Temp->Next != NULL)
								Temp = Temp->Next;
								
							Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
							struct Archive ArcInstance;
							ArcInstance.ID = ReadyQueue->Value->ID;
							ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
							ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime - (ReadyQueue->Value->Runtime - ReadyQueue->Value->RemainingTime);
							ArcInstance.RunTime = ReadyQueue->Value->Runtime;
							ArcInstance.RemainingTime = ReadyQueue->Value->RemainingTime;
							ArcInstance.EventTime = Clk;
							ArcInstance.State = 2;
							
							Temp->Next->ArchiveInstance = ArcInstance;
							Temp->Next->Next = NULL;
							//
						
							kill(ReadyQueue->Value->pid, SIGCONT);
							printf("clk: %d  \t ID: %d will resume\n",getClk(), ReadyQueue->Value->ID);
						}

						runningProcess = ReadyQueue->Value;
						runningProcess->generated = true;

						removeFromReadyQueue(ReadyQueue);
						currQuantum = (runningProcess->RemainingTime > Quantum)? Quantum : runningProcess->RemainingTime;
						PrevClk = getClk();
					}
				}
				else if(runningProcess != NULL){

					if((getClk() - PrevClk) > 0){
						currQuantum -= 1;
						runningProcess->RemainingTime -= 1;
						PrevClk = getClk();
					}

					if(currQuantum == 0){

						if(ReadyQueue != NULL) {

							struct Node* temp = ReadyQueue;
                            printf("Ready Queue: ");
                            while(temp != NULL){
                                printf("%d ", temp->Value->ID);
                                temp = temp->Next;
                            }
                            printf("\n");


							if(runningProcess->RemainingTime > 0){
								//Might Need Semaphore --Archive Related--
								int Clk = getClk();
								struct ArchiveNode* Temp = ArchiveHead;
								while(Temp->Next != NULL)
									Temp = Temp->Next;
									
								Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime - (ReadyQueue->Value->Runtime - ReadyQueue->Value->RemainingTime);
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ReadyQueue->Value->RemainingTime;
								ArcInstance.EventTime = Clk;
								ArcInstance.State = 1;
								
								Temp->Next->ArchiveInstance = ArcInstance;
								Temp->Next->Next = NULL;
								//
							
								struct Node* Node = (struct Node*)malloc(sizeof(struct Node));
								Node->Value = runningProcess;
								Node->Next = NULL;
								addToReadyQueue(Node);
								kill(runningProcess->pid, SIGTSTP);
							}
							

							if(ReadyQueue->Value->generated == false){
								if(fork() == 0){
									char char_arg[100]; 
									sprintf(char_arg, "./process.out %d %d %d", ReadyQueue->Value->Runtime, getppid(), getClk());
									int Status = system(char_arg);
									exit(0);
								}
								
								// Might Need a semaphore
								int Clk = getClk();
								struct ArchiveNode* Temp = ArchiveHead;
								while(Temp->Next != NULL)
									Temp = Temp->Next;
									
								Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime;
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ArcInstance.RunTime;
								ArcInstance.EventTime = Clk;
								ArcInstance.State = 0;

								ArcInstance.memSize = 0;
								
								Temp->Next->ArchiveInstance = ArcInstance;
								Temp->Next->Next = NULL;
								//
								
								usleep(100);
								down(semid_SHC2);
								down(semid_PG2);
								down(semid_SHC1);
								up(semid_PG2);
								up(semid_SHC2);
								int PID = *shmadr_SCH1;
								//printf("OK2, pid: %d\n", PID);
								ReadyQueue->Value->pid = PID;
								ReadyQueue->Value->RemainingTime = ReadyQueue->Value->Runtime;
								printf("clk: %d  \t ID: %d will start\n",getClk(), ReadyQueue->Value->ID);
							}
							else{
								//Might need a semaphore
								//
								int Clk = getClk();
								struct ArchiveNode* Temp = ArchiveHead;
								while(Temp->Next != NULL)
									Temp = Temp->Next;
									
								Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime - (ReadyQueue->Value->Runtime - ReadyQueue->Value->RemainingTime);
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ReadyQueue->Value->RemainingTime;
								ArcInstance.EventTime = Clk;
								ArcInstance.State = 2;
								
								Temp->Next->ArchiveInstance = ArcInstance;
								Temp->Next->Next = NULL;
								//
							
								kill(ReadyQueue->Value->pid, SIGCONT);
								printf("clk: %d  \t ID: %d will resume\n",getClk(), ReadyQueue->Value->ID);
							}

							printf("clk: %d  \t Context Switching: ID: %d will run, ID: %d will sleep.\n",getClk(), ReadyQueue->Value->ID, runningProcess->ID);

							runningProcess = ReadyQueue->Value;
							runningProcess->generated = true;

							removeFromReadyQueue(ReadyQueue);
							currQuantum = (runningProcess->RemainingTime > Quantum)? Quantum : runningProcess->RemainingTime;
						}
						else{
							if(runningProcess->RemainingTime > 0){
								currQuantum = (runningProcess->RemainingTime > Quantum)? Quantum : runningProcess->RemainingTime;
							}
						}
					}
				}
			}
			break;
		case HPF:
			while(1){
				//down(semid_PG_CLK);

				if(ReadyQueue != NULL) {

					struct Node* BestPriority = ReadyQueue;
					struct Node* Temp = ReadyQueue;
					
					while(Temp != NULL){
						if(Temp->Value->Priority < BestPriority->Value->Priority)
							BestPriority = Temp;
							
						Temp = Temp->Next;	
					}


					runningProcess = BestPriority->Value;
					removeFromReadyQueue(BestPriority);
					
					//Might need a semaphore
					//
					//struct memStruct alloc = allocateProcess(BestPriority->Value->memSize, BestPriority->Value->ID);
					int Clk2 = getClk();
					if(ArchiveHead == NULL)
					{
						ArchiveHead = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
						
						struct Archive ArcInstance;
						ArcInstance.ID = runningProcess->ID;
						ArcInstance.ArrivalTime = runningProcess->Arrival;
						ArcInstance.WaitingTime = Clk2 - ArcInstance.ArrivalTime;
						ArcInstance.RunTime = runningProcess->Runtime;
						ArcInstance.RemainingTime = ArcInstance.RunTime;
						ArcInstance.EventTime = Clk2;
						ArcInstance.State = 0;

						ArchiveHead->ArchiveInstance = ArcInstance;
						ArchiveHead->Next = NULL;
					}
					else
					{
						struct ArchiveNode* Temp2 = ArchiveHead;
						while(Temp2->Next != NULL)
							Temp2 = Temp2->Next;
							
						Temp2->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
						struct Archive ArcInstance;
						ArcInstance.ID = runningProcess->ID;
						ArcInstance.ArrivalTime = runningProcess->Arrival;
						ArcInstance.WaitingTime = Clk2 - ArcInstance.ArrivalTime;
						ArcInstance.RunTime = runningProcess->Runtime;
						ArcInstance.RemainingTime = ArcInstance.RunTime;
						ArcInstance.EventTime = Clk2;
						ArcInstance.State = 0;
						
						Temp2->Next->ArchiveInstance = ArcInstance;
						Temp2->Next->Next = NULL;
					}
					//

					if(fork() == 0){
						char char_arg[100]; 
						sprintf(char_arg, "./process.out %d %d %d", BestPriority->Value->Runtime, getppid(), getClk());
						int Status = system(char_arg);
						exit(0);
					}

					usleep(100);
					down(semid_SHC2);
					down(semid_PG2);
					down(semid_SHC1);
					up(semid_PG2);
					up(semid_SHC2);
					int PID = *shmadr_SCH1;
					printf("clk: %d  \t Process of ID: %d starting\n", getClk(), runningProcess->ID);
					runningProcess->pid = PID;

					struct Process RP = *runningProcess;
					sleep(runningProcess->RemainingTime-1);
					
					while(runningProcess){}	
					
					//Might Need Semaphore --Archive Related--
					int Clk3 = getClk();
					struct ArchiveNode* Temp3 = ArchiveHead;
					while(Temp3->Next != NULL)
						Temp3 = Temp3->Next;
						
					Temp3->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
					struct Archive ArcInstance;
					ArcInstance.ID = RP.ID;
					ArcInstance.ArrivalTime = RP.Arrival;
					ArcInstance.WaitingTime = Clk3 - ArcInstance.ArrivalTime - RP.Runtime;
					ArcInstance.RunTime = RP.Runtime;
					ArcInstance.RemainingTime = 0;
					ArcInstance.EventTime = Clk3;
					ArcInstance.State = 3;
					
					Temp3->Next->ArchiveInstance = ArcInstance;
					Temp3->Next->Next = NULL;
					//		
				}
				
			}
			break;
		case SRTN:
			while(1){

				if(runningProcess == NULL){
					if(ReadyQueue != NULL){

						if(ReadyQueue->Value->generated == false){
							
							if(fork() == 0){
								char char_arg[100]; 
								sprintf(char_arg, "./process.out %d %d %d", ReadyQueue->Value->Runtime, getppid(), getClk());
								int Status = system(char_arg);
								exit(0);
							}
							
							//Might need a semaphore
							//
							int Clk4 = getClk();
							if(ArchiveHead == NULL)
							{
								ArchiveHead = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = 0;
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ArcInstance.RunTime;
								ArcInstance.EventTime = Clk4;
								ArcInstance.State = 0;
								
								ArchiveHead->ArchiveInstance = ArcInstance;
								ArchiveHead->Next = NULL;
							}
							else
							{
								struct ArchiveNode* Temp2 = ArchiveHead;
								while(Temp2->Next != NULL)
									Temp2 = Temp2->Next;
									
								Temp2->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
								struct Archive ArcInstance;
								ArcInstance.ID = ReadyQueue->Value->ID;
								ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
								ArcInstance.WaitingTime = 0;
								ArcInstance.RunTime = ReadyQueue->Value->Runtime;
								ArcInstance.RemainingTime = ArcInstance.RunTime;
								ArcInstance.EventTime = Clk4;
								ArcInstance.State = 0;
								
								Temp2->Next->ArchiveInstance = ArcInstance;
								Temp2->Next->Next = NULL;
							}
							//
							
							usleep(100);
							down(semid_SHC2);
							down(semid_PG2);
							down(semid_SHC1);
							up(semid_PG2);
							up(semid_SHC2);
							int PID = *shmadr_SCH1;
							//printf("OK2, pid: %d\n", PID);
							ReadyQueue->Value->pid = PID;
							ReadyQueue->Value->RemainingTime = ReadyQueue->Value->Runtime;
							printf("clk: %d  \t ID: %d will start\n",getClk(), ReadyQueue->Value->ID);
						}
						else{
							//Might need a semaphore
							//
							int Clk = getClk();
							struct ArchiveNode* Temp = ArchiveHead;
							while(Temp->Next != NULL)
								Temp = Temp->Next;
								
							Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
							struct Archive ArcInstance;
							ArcInstance.ID = ReadyQueue->Value->ID;
							ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
							ArcInstance.WaitingTime = Clk - ArcInstance.ArrivalTime - (ReadyQueue->Value->Runtime - ReadyQueue->Value->RemainingTime);
							ArcInstance.RunTime = ReadyQueue->Value->Runtime;
							ArcInstance.RemainingTime = ReadyQueue->Value->RemainingTime;
							ArcInstance.EventTime = Clk;
							ArcInstance.State = 2;
							
							Temp->Next->ArchiveInstance = ArcInstance;
							Temp->Next->Next = NULL;
							//
						
							kill(ReadyQueue->Value->pid, SIGCONT);
							printf("clk: %d  \t ID: %d will resume\n",getClk(), ReadyQueue->Value->ID);
						}

						runningProcess = ReadyQueue->Value;
						runningProcess->generated = true;

						ReadyQueue = ReadyQueue->Next;
					}
				}
				else if(preemption_flag){
					//Might need a semaphore
					//
					int Clk5 = getClk();
					struct ArchiveNode* Temp5 = ArchiveHead;
					while(Temp5->Next != NULL)
						Temp5 = Temp5->Next;
						
					Temp5->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
					struct Archive ArcInstance;
					ArcInstance.ID = ReadyQueue->Value->ID;
					ArcInstance.ArrivalTime = ReadyQueue->Value->Arrival;
					ArcInstance.WaitingTime = Clk5 - ArcInstance.ArrivalTime - (ReadyQueue->Value->Runtime - ReadyQueue->Value->RemainingTime);
					ArcInstance.RunTime = ReadyQueue->Value->Runtime;
					ArcInstance.RemainingTime = ReadyQueue->Value->RemainingTime;
					ArcInstance.EventTime = Clk5;
					ArcInstance.State = 1;
					
					Temp5->Next->ArchiveInstance = ArcInstance;
					Temp5->Next->Next = NULL;
					//
				
					kill(runningProcess->pid, SIGTSTP);
					if(fork() == 0){
						char char_arg[100]; 
						sprintf(char_arg, "./process.out %d %d %d", ReadyQueue->Value->Runtime, getppid(), getClk());
						int Status = system(char_arg);
						exit(0);
					}
					
					//
					int Clk6 = getClk();
					struct ArchiveNode* Temp6 = ArchiveHead;
					while(Temp6->Next != NULL)
						Temp6 = Temp6->Next;
						
					Temp6->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
					struct Archive ArcInstance2;
					ArcInstance2.ID = ReadyQueue->Value->ID;
					ArcInstance2.ArrivalTime = ReadyQueue->Value->Arrival;
					ArcInstance2.WaitingTime = 0;
					ArcInstance2.RunTime = ReadyQueue->Value->Runtime;
					ArcInstance2.RemainingTime = ArcInstance2.RunTime;
					ArcInstance2.EventTime = Clk6;
					ArcInstance2.State = 0;
					ArcInstance.memSize = 0;
					
					Temp6->Next->ArchiveInstance = ArcInstance2;
					Temp6->Next->Next = NULL;
					//

					usleep(100);
					down(semid_SHC2);
					down(semid_PG2);
					down(semid_SHC1);
					up(semid_PG2);
					up(semid_SHC2);
					int PID = *shmadr_SCH1;
					//printf("OK2, pid: %d\n", PID);
					ReadyQueue->Value->pid = PID;
					ReadyQueue->Value->RemainingTime = ReadyQueue->Value->Runtime;

					printf("clk: %d  \t Context Switching: ID: %d will run, ID: %d will sleep.\n",getClk(), ReadyQueue->Value->ID, runningProcess->ID);

					struct Process* tempRQ = ReadyQueue->Value;
					ReadyQueue->Value = runningProcess;
					runningProcess = tempRQ;
					runningProcess->generated = true;

					preemption_flag = false;
				}
				else if(runningProcess != NULL && (getClk() - PrevClk) >= 1){
					runningProcess->RemainingTime -= 1;
				}

				PrevClk = getClk();
			}
			break;
	}
    
    
    CleanUp(0);
}

void ProcessArrived(int signum) //Process generator signals the scheduler that there is a process arrived and should be taken from the shared memory
{
	struct Node* NewNode = (struct Node*)malloc(sizeof(struct Node));
	NewNode->Next = NULL;
	NewNode->Value = (struct Process*)malloc(sizeof(struct Process));
	*(NewNode->Value) = *((struct Process*)shmadr_PG1);

	printf("clk: %d  \t Sch Rec: id: %d, arr: %d, runtime: %d, p: %d.\n", getClk(),
	 NewNode->Value->ID, NewNode->Value->Arrival, NewNode->Value->Runtime, NewNode->Value->Priority);
	///
	struct memStruct alloc = allocateProcess(NewNode->Value->memSize, NewNode->Value->ID);
	printf("Alloc : id: %d , start: %d, end: %d \n",alloc.id,alloc.start,alloc.end);
	//if(alloc.id==-1)
	//	continue;
	///
	int Clk = getClk();
	if(ArchiveHead2 == NULL)
	{
		ArchiveHead2 = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
		
		struct Archive ArcInstance;
		ArcInstance.ID = NewNode->Value->ID;
		ArcInstance.EventTime = Clk;
		ArcInstance.State = 0;
		ArcInstance.startMemIndex = alloc.start;
		ArcInstance.endMemIndex = alloc.end;
		ArcInstance.memSize = NewNode->Value->memSize;
		
		ArchiveHead2->ArchiveInstance = ArcInstance;
		ArchiveHead2->Next = NULL;
	}
	else
	{
		
		struct ArchiveNode* Temp = ArchiveHead2;
		while(Temp->Next != NULL)
			Temp = Temp->Next;
			
		Temp->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
		struct Archive ArcInstance;
		ArcInstance.ID = NewNode->Value->ID;
		ArcInstance.EventTime = Clk;
		ArcInstance.State = 0;

		ArcInstance.startMemIndex = alloc.start;
		ArcInstance.endMemIndex = alloc.end;
		ArcInstance.memSize = NewNode->Value->memSize;
		
		Temp->Next->ArchiveInstance = ArcInstance;
		Temp->Next->Next = NULL;
	}
	///
	
	if(SchedulingAlgorithm == HPF || SchedulingAlgorithm == RR){

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
		return;
	}
	else if(SchedulingAlgorithm == SRTN){

		if(runningProcess != NULL){
			if(NewNode->Value->Runtime < runningProcess->RemainingTime){
				NewNode->Next = ReadyQueue;
				ReadyQueue = NewNode;
				preemption_flag = true;
				up(semid_PG1);
				return;
			}
		}
		
		if(ReadyQueue == NULL){
			ReadyQueue = NewNode;
			up(semid_PG1);
			return;
		}
		
		struct Node* Temp = ReadyQueue;

		if(NewNode->Value->Runtime < ReadyQueue->Value->Runtime){
			NewNode->Next = ReadyQueue;
			ReadyQueue = NewNode;
			up(semid_PG1);
			return;
		}
		
		while(Temp->Next != NULL && NewNode->Value->Runtime > Temp->Value->Runtime){
			Temp = Temp->Next;
		}
		NewNode->Next = Temp->Next;
		Temp->Next = NewNode;
	}

	up(semid_PG1);
}

void ProcessFinished(int signum){

	///
	struct memStruct mdealloc = deallocateProcess(runningProcess->memSize, runningProcess->ID);
	printf("Dealloc - id: %d , start: %d, end: %d \n",mdealloc.id,mdealloc.start,mdealloc.end);
	///

	runningProcess->finishTime = getClk();
	processFinished_flag = true;
	
		int Clk6 = getClk();
		struct ArchiveNode* Temp6;
	if(SchedulingAlgorithm == SRTN || SchedulingAlgorithm == RR)
	{
		//
		Temp6 = ArchiveHead;
		while(Temp6->Next != NULL)
			Temp6 = Temp6->Next;
			
		Temp6->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
		struct Archive ArcInstance2;
		ArcInstance2.ID = runningProcess->ID;
		ArcInstance2.ArrivalTime = runningProcess->Arrival;
		ArcInstance2.WaitingTime = Clk6 - ArcInstance2.ArrivalTime - (runningProcess->Runtime - runningProcess->RemainingTime);
		ArcInstance2.RunTime = runningProcess->Runtime;
		ArcInstance2.RemainingTime = 0;
		ArcInstance2.EventTime = Clk6;
		ArcInstance2.State = 3;
		
		Temp6->Next->ArchiveInstance = ArcInstance2;
		Temp6->Next->Next = NULL;
		//
	}
//mem manager
		Temp6 = ArchiveHead2;
		while(Temp6->Next != NULL)
			Temp6 = Temp6->Next;
			
		Temp6->Next = (struct ArchiveNode*)malloc(sizeof(struct ArchiveNode));
		struct Archive ArcInstance2;
		ArcInstance2.ID = runningProcess->ID;
		ArcInstance2.EventTime = Clk6;
		ArcInstance2.State = 3;

		ArcInstance2.startMemIndex=mdealloc.start;
		ArcInstance2.endMemIndex = mdealloc.end;
		ArcInstance2.memSize = runningProcess->memSize;
		
		Temp6->Next->ArchiveInstance = ArcInstance2;
		Temp6->Next->Next = NULL;

	printf("clk: %d  \t Procces %d has finished :)\n", runningProcess->finishTime, runningProcess->ID);

	runningProcess = (SchedulingAlgorithm==RR)? runningProcess : NULL;
}

void addToArchive(struct Process* proc){
	struct Node* newArck = (struct Node*) malloc(sizeof(struct Node));
	newArck->Value = (struct Process*) malloc(sizeof(struct Process));

	*(newArck->Value) =  *proc;

	if(archive == NULL){
		archive = newArck;
		archive_tail = archive;
	}
	else{
		archive_tail->Next = newArck;
		archive_tail = archive_tail->Next;
	}
}

void removeFromReadyQueue(struct Node* node){

	struct Node* temp = ReadyQueue;

	if(node == ReadyQueue){
		ReadyQueue = ReadyQueue->Next;
		node->Next = NULL;
		return;
	}

	while(temp->Next != NULL){
		if(node == temp->Next){
			temp->Next = node->Next;
			node->Next = NULL;
			return;
		}
		temp=temp->Next;
	}
}

void addToReadyQueue(struct Node* node){
	
	if(ReadyQueue == NULL){
		ReadyQueue = node;
		ReadyQueue->Next = NULL;
	}
	else{
		struct Node* Temp = ReadyQueue;
		
		while(Temp->Next != NULL){
			Temp = Temp->Next;
		}

		Temp->Next = node;
	}
}

/* Clear the resources before exit */
void CleanUp(int signum)
{
	int Counter = 0;
	float AvgWait = 0;
	float AvgWTA = 0;
	int TotalRunTime = 0;
	int MaxFT = 0;
	float CPU_Utilizatiion = 0;
	float StandardDev = 0;

	FILE * pFile; //Openning the procceses file for reading
	pFile = fopen("scheduler.log", "w+");
	
	while(1)  //Writing Output file
	{
		fprintf(pFile, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
		struct ArchiveNode* Temp = ArchiveHead;
		while(Temp != NULL)
		{
			char Word[20];
			if(Temp->ArchiveInstance.State == 0)
				sprintf(Word, "started");
			else if(Temp->ArchiveInstance.State == 1)
				sprintf(Word, "stopped");
			else if(Temp->ArchiveInstance.State == 2)
				sprintf(Word, "resumed");
			else if(Temp->ArchiveInstance.State == 3)
			{
				sprintf(Word, "finished");
				
				AvgWait += Temp->ArchiveInstance.WaitingTime;
				AvgWTA += (Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.RunTime)/Temp->ArchiveInstance.RunTime;
				TotalRunTime += Temp->ArchiveInstance.RunTime;
				
				if(MaxFT < (Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.ArrivalTime + Temp->ArchiveInstance.RunTime))
					MaxFT = (Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.ArrivalTime + Temp->ArchiveInstance.RunTime);
				Counter++;
			}
		
			fprintf(pFile, "At\ttime\t%d\tprocess\t%d\t%s\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d", Temp->ArchiveInstance.EventTime, Temp->ArchiveInstance.ID, Word, Temp->ArchiveInstance.ArrivalTime, Temp->ArchiveInstance.RunTime, Temp->ArchiveInstance.RemainingTime, Temp->ArchiveInstance.WaitingTime);

            if(Temp->ArchiveInstance.State == 3)
                fprintf(pFile, "\tTA\t%d\tWTA\t%0.2f", (Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.RunTime), (float)(Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.RunTime)/(Temp->ArchiveInstance.RunTime));

            fprintf(pFile, "\n");

            Temp = Temp->Next;
		}
		
		break;
  	}
  	
  	fclose(pFile);

	pFile = fopen("memory.log", "w+");

	while (1) //Writing Output file
	{
		fprintf(pFile, "#At time x allocated y bytes for process z from i to j\n");
		struct ArchiveNode *TempMem = ArchiveHead2;
		while (TempMem != NULL)
		{
			char Word[20];
			if (TempMem->ArchiveInstance.State == 0  && TempMem->ArchiveInstance.endMemIndex<=1024&& TempMem->ArchiveInstance.endMemIndex!=0) //allocated
			{
				sprintf(Word, "allocated");
				fprintf(pFile, "At time %d %s %d bytes for process %d from %d to %d\n", TempMem->ArchiveInstance.EventTime, Word, TempMem->ArchiveInstance.memSize, TempMem->ArchiveInstance.ID, TempMem->ArchiveInstance.startMemIndex, TempMem->ArchiveInstance.endMemIndex);
			}

			else if (TempMem->ArchiveInstance.State == 3&& TempMem->ArchiveInstance.endMemIndex<=1024&& TempMem->ArchiveInstance.endMemIndex!=0) //freed
			{
				sprintf(Word, "freed");
				fprintf(pFile, "At time %d %s %d bytes for process %d from %d to %d\n", TempMem->ArchiveInstance.EventTime, Word, TempMem->ArchiveInstance.memSize, TempMem->ArchiveInstance.ID, TempMem->ArchiveInstance.startMemIndex, TempMem->ArchiveInstance.endMemIndex);
			}

			TempMem = TempMem->Next;
		}

		break;
	}

	fclose(pFile);

  	
  	AvgWait /= Counter;
  	AvgWTA /= Counter;
  	CPU_Utilizatiion = ((float)TotalRunTime / MaxFT) * 100;
  	
  	struct ArchiveNode* Temp = ArchiveHead;
	while(Temp != NULL)
	{
		if(Temp->ArchiveInstance.State == 3)
		{
			float Temp2 = (Temp->ArchiveInstance.WaitingTime + Temp->ArchiveInstance.RunTime)/(Temp->ArchiveInstance.RunTime) - AvgWTA;
			StandardDev += (Temp2*Temp2);
		}
		
		Temp = Temp->Next;
	}
	
	StandardDev = sqrtf(StandardDev);
  	
  	FILE * pFile2; //Openning the procceses file for reading
	pFile2 = fopen("scheduler.perf", "w+");
	
	fprintf(pFile2, "CPU\tutilization\t=\t%0.2f%%\n", CPU_Utilizatiion);
	fprintf(pFile2, "Avg\tWTA\t=\t%0.2f\n", AvgWTA);
	fprintf(pFile2, "Avg\tWaiting\t=\t%0.2f\n", AvgWait);
	fprintf(pFile2, "Std\tWTA\t=\t%0.2f", StandardDev);
  	
  	fclose(pFile2);
	
  	Temp = ArchiveHead;
  	while(Temp != NULL)
  	{
  		struct ArchiveNode* ARC2 = Temp;
  		Temp = Temp->Next;
  		
  		free(ARC2);
  	}

	shmctl(shmid_SCH1, IPC_RMID, NULL);
	semctl(semid_SHC1, IPC_RMID, 0);
	semctl(semid_SHC2, IPC_RMID, 0);
    destroyClk(true);
    printf("Scheduler terminating!\n");
    exit(0);
}

void createAttachResources(){

	shmid_PG1 = shmget(SHKEYPROCESS2, sizeof(struct Process), 0666);
    if ((long)shmid_PG1 == -1){
        perror("Error in creating shm! in Scheduler!");
        exit(-1);
    }
    shmadr_PG1 = (struct Process*) shmat(shmid_PG1, (void *)0, 0);
    if ((long)shmadr_PG1 == -1){
        perror("Error in attaching the shm in Scheduler!");
        exit(-1);
    }

	shmid_PG2 = shmget(1279456, sizeof(pid_t), 0666);
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
	semid_PG2 = semget(8934532, 1, 0666 | IPC_CREAT);
	if ((long)semid_PG2 == -1){
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

	semid_SHC2 = semget(8456745, 1, IPC_CREAT | 0666);
	if ((long)semid_SHC2 == -1){
        perror("Error in creating sem! in Scheduler!");
        exit(-1);
    }
	semun.val = 1; /* initial value of the semaphore, Binary semaphore */
    if (semctl(semid_SHC2, 0, SETVAL, semun) == -1){
        perror("Error in semaphore");
        exit(-1);
    }

	semid_CLK = semget(980923, 1, 0666);
	if ((long)semid_CLK == -1){
        perror("Error in creating sem! in Sch!");
        exit(-1);
    }

	

	semid_PG_CLK = semget(345645, 1, 0666);
	if ((long)semid_PG_CLK == -1){
        perror("Error in creating sem! in sch!");
        exit(-1);
    }
}
