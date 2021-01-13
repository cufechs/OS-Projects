#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <ctype.h>


#define BUF_SIZE 15

struct SharedBuff* buff; //holds the addrees of the shared memory space that is of type SharedBuff
int shmid; //shared memory id
struct shmid_ds shmid_buff; //to know the number of attached process to the shared memory
int mutex, empty, full;

/* arg for semctl system calls. */
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

/* shared memory to produce and consume in/from */
struct SharedBuff {
	int buffer[BUF_SIZE]; 		//the actual buffer that we will produce into and consume from
	int current_idx; 			//current index in the buffer
	int current_item; 			//curretn item produced 

	int current_consume_idx; 	//current consumed index
	int current_consume_item; 	//current consumed item
}; 


void down(int sem) // sem: semaphore id
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem) // sem: semaphore id
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}


void handler(int signum) {

	shmctl(shmid, IPC_STAT, &shmid_buff);

	//printf("number of attached processes: %d\n",(int) shmid_buff.shm_nattch);
	
	// detach shared memory from process address space
	shmdt(buff);

	//Check if this is the last attached process
	if ((int) shmid_buff.shm_nattch == 1) {
		// 1. delete shared memory
		// 2. delete all semaphores	
		shmctl(shmid, IPC_RMID, (struct shmid_ds*)0);
		semctl(mutex, IPC_RMID, (struct shmid_ds*)0);
		semctl(empty, IPC_RMID, (struct shmid_ds*)0);
		semctl(full, IPC_RMID, (struct shmid_ds*)0);
	} 

	printf("\n");
	exit(0);

}



int main() {

	// ********************************* Create Semaphore *********************************
	union Semun semun;

	// semaphore for handling multiple producers
	mutex = semget(1234, 1, 0666 | IPC_CREAT | IPC_EXCL);
	empty = semget(1235, 1, 0666 | IPC_CREAT | IPC_EXCL);
	full = semget(1236, 1, 0666 | IPC_CREAT | IPC_EXCL);

	if (mutex == -1) 
    {
        //already created, therefore only get the id
        mutex = semget(1234, 1, 0666 | IPC_CREAT);
    } else {
        //not created, therefore initialize it 
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(mutex, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }

    if (empty == -1) 
    {
        //already created, therefore only get the id
        empty = semget(1235, 1, 0666 | IPC_CREAT);
    } else {
        //not created, therefore initialize it 
        semun.val = BUF_SIZE; /* initial value of the semaphore, size of the buffer */
        if (semctl(empty, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }

    if (full == -1) 
    {
        //already created, therefore only get the id
        full = semget(1236, 1, 0666 | IPC_CREAT);
    } else {
        //not created, therefore initialize it 
        semun.val = 0; /* initial value of the semaphore, nothing is full now */
        if (semctl(full, 0, SETVAL, semun) == -1)
        {
            perror("Error in semctl");
            exit(-1);
        }
    }

	// ************************* Create Shared Memory or get existing id ******************************
	shmid = shmget(1244, sizeof(struct SharedBuff), IPC_CREAT | 0644);
	if (shmid == -1) {
		perror("Error in create");
		exit(-1); // TODO: check this!
	} else {
		printf("\nShared Memory ID = %d\n", shmid);
	}

	// *********************** Attach shared memory to the address space **********************
	// *** write to shared memory ***
	//attach shared memory to current process
	buff = shmat(shmid, (void *)0, 0);
	printf("buffer %d\n", buff);
    if (buff == -1)
    {
        perror("Error in attach");
        exit(-1); // TODO: check this
    }
    else
    {	
        printf("\nShared memory attached at address %x\n", buff);
    }


    signal(SIGINT, handler);
    
    // ************************ Shared Memory Initialization ***********************
    // //down(mutex);
    // if (buff->current_idx > 9999) { // TODO: make a better check for grabage values
    // 	//initialize the buffer
    // 	buff->current_idx = 0;
    // 	buff->current_item = 1;
    // 	printf("Initializeing shared memroy\n");
    // }
    // up(mutex);

    while (1) {

    	printf("Producing an item...\n");
    	down(empty);
    	down(mutex);
    	
    	buff->current_item++;
    	printf("Producing item number: %d current index=%d\n\n", buff->current_item,buff->current_idx%BUF_SIZE);
    	buff->buffer[buff->current_idx % BUF_SIZE] = buff->current_item; 
    	
    	buff->current_idx++;

    	up(mutex);
    	up(full);
    	sleep(2);

    }

    shmdt(buff);
    return 0;


}