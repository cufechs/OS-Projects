#include "headers.h"

/* Modify this file as needed*/
int remainingtime;

int main(int agrc, char * argv[])
{
    initClk();
    
    //TODO it needs to get the remaining time from somewhere
    //remainingtime = ??; //We should use shared memory to get the remaining time of the process
    while (remainingtime > 0)
    {
        // remainingtime = ??;
    }
    
    //We should notify the scheduler that this process finished TODO
    destroyClk(false);
    
    return 0;
}
