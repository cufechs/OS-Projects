#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int Clock;

void WokeUp(int signum)
{
	//TODO
	Clock = getClk();
}

void StopMe(int signum)
{
	if(getClk() - Clock == 1)
		remainingtime--;
		
	if(remainingtime > 0)
		kill(SIGSTOP, getpid());
}

int main(int agrc, char * argv[])
{
	singal(SIGCONT, WokeUp);
	singal(SIGTSTP, StopMe);
	int PID_SCHD = 0;
    initClk();
    Clock = getClk();
    //TODO it needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]); //We should use shared memory to get the remaining time of the process
    while (remainingtime > 0)
    {
    	//Semaphore
    	if(getClk() - Clock == 1)
    	{
    		remainingtime--;
    		Clock = getClk();
    	}
    }
    
    //We should notify the scheduler that this process finished TODO
    //Sempahore
    
    PID_SCHD = atoi(argv[2]);
    kill(PID_SCHD, SIGUSR2);
    destroyClk(false);
    
    return 0;
}
