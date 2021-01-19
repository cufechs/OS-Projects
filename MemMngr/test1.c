#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main()
{
	int shmid_PG1;
	int* shmadr_PG1;
	int shmid_PG2;
	pid_t* shmadr_PG2;
	int semid_PG1;

	shmid_PG1 = shmget(321231, sizeof(int*), IPC_CREAT | 0666);
    if ((long)shmid_PG1 == -1)
    {
        shmid_PG1 = shmget(321231, sizeof(int*), IPC_CREAT | 0666);
    }
    shmadr_PG1 = (int*) shmat(shmid_PG1, (void *)0, 0);
    *shmadr_PG1 = 5;
    
    sleep(15);
}
