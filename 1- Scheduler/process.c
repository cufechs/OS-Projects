#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int Clock;
int PID_SCHD;

int shmid_SCH1;
pid_t* shmadr_SCH1;
int semid_SHC1;

void WokeUp(int signum);
void StopMe(int signum);
void attachResources();

int main(int agrc, char * argv[])
{
	signal(SIGCONT, WokeUp);
	signal(SIGTSTP, StopMe);

	attachResources();

	*shmadr_SCH1 = getpid();
    //printf("OK1, pid: %d\n", *shmadr_SCH1);
	up(semid_SHC1);

    initClk();
    Clock = getClk();
    //TODO it needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]); //We should use shared memory to get the remaining time of the process
    //printf("will run for %d sec.\n", remainingtime);
    
    while (remainingtime > 0){
    	//Semaphore
    	if(getClk() - Clock == 1){
    		remainingtime--;
            //printf("1 Sec passed and my remaining time: %d,,,,,clock %d\n",remainingtime, getClk());
    		Clock = getClk();
    	}
    }
    
    //We should notify the scheduler that this process finished TODO
    //Sempahore
    
    PID_SCHD = atoi(argv[2]);
    kill(PID_SCHD, SIGUSR2);
    destroyClk(false);
    
	exit(0);
    return 0;
}

void WokeUp(int signum){
	//TODO
    printf("clk: %d \t I am resuming and my remaining time: %d\n", getClk(),remainingtime);
	Clock = getClk();
}

void StopMe(int signum){

	if(getClk() - Clock == 1)
		remainingtime--;
		
    printf("clk: %d \t I am stopped and my remaining time: %d\n", getClk(), remainingtime);

	if(remainingtime > 0){
		kill(getpid(), SIGSTOP);
    }
}

void attachResources(){

	shmid_SCH1 = shmget(53242, sizeof(pid_t), 0666);
    if ((long)shmid_SCH1 == -1){
        perror("Error in creating shm! in process generator!");
        exit(-1);
    }
    shmadr_SCH1 = (pid_t*) shmat(shmid_SCH1, (void *)0, 0);
    if ((long)shmadr_SCH1 == -1){
        perror("Error in attaching the shm in process generator!");
        exit(-1);
    }

	semid_SHC1 = semget(512432, 1, 0666);
	if ((long)semid_SHC1 == -1){
        perror("Error in creating sem! in Scheduler!");
        exit(-1);
    }
}