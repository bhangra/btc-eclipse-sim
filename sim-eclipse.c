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
#include"record.c"
#include"params.h"

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int /*miner_id,*/ i;
	struct threads *threads;
//	struct killed	*killed, *k_next;

	dead = NULL;

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
		threads=new_thread(1, miner_id, threads, 1);
	}
	keep_total_hash_rate_1(threads);

/////////////////////////////////////////////////////////////////////
// the main routine	
/////////////////////////////////////////////////////////////////////
// sim_time=1 : 1 second
	for(sim_time = 0; sim_time < SIM_TIME; sim_time++){
#ifdef DEBUG
		fprintf(stderr, "sim_time = %d\n", sim_time);//debug
#endif
/*		if(dead!=NULL){
			for(killed=dead; killed!=NULL; killed=k_next){
				dead=NULL;
				k_next=killed->next;
				free(killed);
			}
		}
*/
// kill/create nodes, manage total hash-rate
		threads= cancel_by_TTL(threads);
		keep_total_seeds(threads);
		keep_total_nodes(threads);
		keep_total_hash_rate_1(threads);
		if(sim_time%600==0){
			add_link_records(threads);
		}
// routine
		for(;threads->next!=NULL; threads=threads->next){}
		for(;;threads=threads->prev){
			if(threads->type==ATTACKER){
				bad_miner_routine(threads->miner);
			}
			else{
				if(sim_time>=SIM_TIME-1){
//#ifdef DEBUG
					fprintf(stderr, "\nminer: %d\n", (threads->miner)->miner_id);
					fprintf(stderr, "created at: %d\n", threads->time);
//#endif
				}
				miner_routine(threads->miner);
			}
			if(threads->prev==NULL)
				break;
		}
		for(i=0; i<NUM_DNS; i++){
			dns_routine(&dns[i]);
		}
	}
	fprintf(stderr, "will cancel_all()\n"); //debug
	cancel_all(threads);
//	print_link_record();
	print_block_record();
	exit(1);
}

#endif
