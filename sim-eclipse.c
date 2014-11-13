#ifndef SIM_ECLIPSE_C
#define SIM_ECLIPSE_C


#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<time.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

//#include"thread.c"
//#include"node.c"
#include"routine.c"


#define BAD_DNS 0
#define ATTACKER 0
#define INIT_NODES 1000

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int times, miner_id, i;
	struct threads *threads;

// random seed
	srand(time(NULL));

//DNS nodes initialization
	memset(&dns, 0, 5*sizeof(struct dns));


	threads = NULL;
	for(miner_id=0; miner_id<5; miner_id++){
		threads=new_thread(1, miner_id, threads);
	}
//	thread = new_thread(1, NULL);
	for(times = 0; times < 1000; times++){
		fprintf(stderr, "times = %d\n", times);//debug
		for(;threads->prev!=NULL; threads=threads->prev){}
		for(;;threads=threads->next){
			fprintf(stderr, "\nminer: %d\n", (threads->miner)->miner_id);
			miner_routine(threads->miner);
			if(threads->next==NULL)
				break;
		}
		for(i=0; i<5; i++){
			dns_routine(&dns[i]);
		}
	}
	fprintf(stderr, "will cancel_all()\n"); //debug
	cancel_all(threads);
	return EXIT_SUCCESS;
}

#endif
