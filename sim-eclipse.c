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
//#include<mcheck.h>

//#include"thread.c"
//#include"node.c"
#include"routine.c"
#include"bad-routine.c"
#include"record.c"
#include"params.h"

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int /*miner_id,*/ i;//, num_nodes;
	struct threads *threads;
//	mcheck(NULL);

	dead = NULL;
	global_id = 0;
// random seed
	srand(time(NULL));

//nodes list initialization
	threads = NULL;

//DNS nodes initialization 
	memset(&dns, 0, NUM_DNS*sizeof(struct dns)); //will delete?

//Seeds Initialization
 	for(i=0; i<SEED_NUM; i++){
		memset(&seeds[i], 0, sizeof(seeds[i]));
		seeds[i].TTL = SIM_TIME;
		seeds[i].miner_id = global_id;
		global_id++;
	}

//bad nodes/DNS initialization
	for(i=0; i<NUM_DNS; i++){
		is_bad_dns[i] = false;
	}
	bad_links = NULL;

#ifdef BAD_NODES
	unsigned int num_nodes;
	for(num_nodes=0; num_nodes<BAD_NODES; global_id++){
		threads=new_thread(ATTACKER, global_id, threads, 0);
		num_nodes++;
	}
#endif 


//initial seed nodes creation
/*	for(num_nodes=0; num_nodes<SEED_NUM; global_id++){
		threads=new_thread(HONEST, global_id, threads, 1);
		num_nodes++;
	}
	keep_total_hash_rate_1(threads);
*/
/////////////////////////////////////////////////////////////////////
// the main routine	
/////////////////////////////////////////////////////////////////////
// sim_time=1 : 1 second
	for(sim_time = 0; sim_time < SIM_TIME; sim_time++){
#ifdef DEBUG
		fprintf(stderr, "sim_time = %d\n", sim_time);//debug
#endif

#ifdef DEBUG
		struct killed *killed;
		fprintf(stderr, "dead nodes: ");
		for(killed = dead; killed!=NULL; killed = killed->next){
			fprintf(stderr, "id= %d ", killed->id);
		}
		fprintf(stderr, "\n");
#endif

// kill/create nodes, manage total hash-rate
		threads = cancel_by_TTL(threads);
//		threads = keep_total_seeds(threads);
		threads = keep_total_nodes(threads);
		keep_total_hash_rate_1(threads);
		if(sim_time%600==0){
			add_link_records(threads);
		}
// routine
		for(;threads->next!=NULL; threads=threads->next){}
		for(;;threads=threads->prev){
#ifdef DEBUG
		fprintf(stderr, "threads->miner->miner_id= %d\n", threads->miner->miner_id);
#endif
			if(threads->miner->seed == true){
#ifdef DEBUG
					fprintf(stderr, "\nminer: %d seed: %d\n", (threads->miner)->miner_id, threads->miner->seed);
					fprintf(stderr, "created at: %d\n", threads->time);
#endif
					miner_routine(threads->miner);

			}
			else{
				if(threads->type==ATTACKER){
					bad_miner_routine(threads->miner);
				}
				else{
//				if(sim_time>=SIM_TIME-1){
#ifdef DEBUG
						fprintf(stderr, "\nminer: %d seed: %d\n", (threads->miner)->miner_id, threads->miner->seed);
						fprintf(stderr, "created at: %d\n", threads->time);
#endif
//				}
					miner_routine(threads->miner);
				}
			}
			if(threads->prev==NULL)
				break;
		}
		for(i=0; i<SEED_NUM; i++){
#ifdef DEBUG
						fprintf(stderr, "\nminer: %d seed: %d\n", seeds[i].miner_id, seeds[i].seed);
#endif

			miner_routine(&seeds[i]);
		}
		for(i=0; i<NUM_DNS; i++){
#ifdef DEBUG
			fprintf(stderr, "dns[%d]\n", i);
#endif
			dns_routine(&dns[i]);
		}
	}
#ifdef DEBUG
	fprintf(stderr, "will cancel_all()\n"); //debug
#endif
	cancel_all(threads);
	free_killed();
	print_link_record();
	print_block_record();
	exit(1);
}

#endif
