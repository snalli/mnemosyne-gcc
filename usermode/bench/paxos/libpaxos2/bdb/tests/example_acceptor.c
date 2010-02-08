#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#define __USE_GNU
#include <sched.h>

#include "libpaxos.h"

void handle_cltr_c (int sig) {
	printf("Caught signal %d\n", sig);
    acceptor_exit();
    exit(0);
}

int main (int argc, char const *argv[]) {

    signal(SIGINT, handle_cltr_c);
    
    if (argc != 2) {
        printf("This program takes exactly one argument: the acceptor unique identifier\n");
        exit(1);
    }
    
    short int acceptor_id = atoi(argv[1]);
    
    if (acceptor_init(acceptor_id) != 0) {
        printf("Could not start the acceptor!\n");
        exit(1);
    }
	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(acceptor_id, &cpuset);
	//if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) < 0) {
//		perror("sched_setaffinity");
 //   }

    while(1) {
        //This thread does nothing...
        //But it can't terminate!
        sleep(1);
    }

    return 0;
}
