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
#include"params.h"
/*
#define SIM_DAYS	1
#define SIM_TIME	60*60*24*SIM_DAYS
#define NUM_DNS		5
#define BAD_DNS		0
#define ATTACKER	0
#define INIT_NODES	1000
#define SEED_NUM	10
*/
//global variables:
//		struct dns dns[]; proto-node.h

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int times, miner_id, i;
	struct threads *threads;

// random seed
	srand(time(NULL));

//nodes list initialization
	threads = NULL;

//DNS nodes initialization
	memset(&dns, 0, NUM_DNS*sizeof(struct dns));

//bad nodes/DNS initialization
	for(i=0; i<NUM_DNS; i++){
		is_bad_dns[i] = false;
	}
	bad_links = NULL;

//initial seed nodes creation
	for(miner_id=0; miner_id<SEED_NUM; miner_id++){
		threads=new_thread(1, miner_id, threads, 1, times);
	}
	keep_total_hash_rate_1(threads);

// the main routine	
// times=1 : 1 second
	for(times = 0; times < SIM_TIME; times++){
		fprintf(stderr, "times = %d\n", times);//debug
// killing/creating nodes, managing total hash-rate
		for(;threads->next!=NULL; threads=threads->next){}
		cancel_by_TTL(times, threads);
		keep_total_hash_rate_1(threads);

// routine
		for(;;threads=threads->prev){
			fprintf(stderr, "\nminer: %d\n", (threads->miner)->miner_id);
			miner_routine(threads->miner);
			if(threads->prev==NULL)
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
