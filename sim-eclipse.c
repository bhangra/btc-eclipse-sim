#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#include"thread.c"
#include"node.c"
#include"routine.c"


#define BAD_DNS 0
#define ATTACKER 0
#define INIT_NODES 1000

int main(int argc, char *argv[]){
	struct dns	dns[5];
	int time;
	struct threads *thread;
	thread = new_thread(0, NULL);
	for(time = 0; time < 5; time++){
		fprintf(stderr, "time = %d\n", time);//debug
		miner_routine(thread->miner);
	}
	cancel_all(thread);
	return EXIT_SUCCESS;
}
