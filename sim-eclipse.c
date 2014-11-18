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
#include"bad-routine.c"

#define NUM_DNS 5
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
	memset(&dns, 0, NUM_DNS*sizeof(struct dns));
	for(i=0; i<NUM_DNS; i++){
		is_bad_dns[i] = false;
	}
	bad_links = NULL;

	threads = NULL;
	for(miner_id=0; miner_id<INIT_NODES; miner_id++){
		threads=new_thread(1, miner_id, threads);
	}
//	thread = new_thread(1, NULL);
	for(times = 0; times < 50; times++){
		fprintf(stderr, "times = %d\n", times);//debug
		for(;threads->prev!=NULL; threads=threads->prev){}
		for(;;threads=threads->next){
			fprintf(stderr, "\nminer: %d\n", (threads->miner)->miner_id);
			miner_routine(threads->miner);
			if(threads->next==NULL)
				break;
		}
		for(i=0; i<NUM_DNS; i++){
			dns_routine(&dns[i]);
		}
	}
	fprintf(stderr, "will cancel_all()\n"); //debug
	cancel_all(threads);
	return EXIT_SUCCESS;
}

#endif
