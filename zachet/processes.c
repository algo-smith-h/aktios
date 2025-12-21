//Даны процессы, надо передать сигнал SIGUSR1 группе: 
//1 -> (2, 3, 4). Все процессы проинициализированы
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {

    pid_t pid2 = atoi(argv[1]);  
    pid_t pid3 = atoi(argv[2]);  
    pid_t pid4 = atoi(argv[3]);  
    
    printf("(PID: %d) send SIGUSR1...\n",
         getpid());
    
    kill(pid2, SIGUSR1);
    kill(pid3, SIGUSR1);
    kill(pid4, SIGUSR1);
    
    return 0;
}
