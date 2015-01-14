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

#ifdef MULTI
struct thread_arg{
	int num;
	struct threads *thread;
};
void *thread_job(struct thread_arg *a);

#endif

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int /*miner_id,*/ i;//, num_nodes;
	struct threads *threads=NULL; 
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
//	fprintf(stderr, "seeds' subnet\n");
	int subnet = rand() & 0xffff0000;
 	for(i=0; i<SEED_NUM; i++){
		if(i>SEED_NUM/2 && i <= SEED_NUM/2 +1)
			subnet = rand() & 0xffff0000;
		seeds[i] = malloc(sizeof(struct miner));
		memset(seeds[i], 0, sizeof(struct miner));
		memset(&seeds[i]->addrman, 0, sizeof(struct addrman));
		seeds[i]->seed		= true;
		seeds[i]->TTL		= SIM_TIME;
		seeds[i]->miner_id	= global_id;
		global_id++;
		seeds[i]->subnet		= subnet;
//		fprintf(stderr, "subnet: %d\n", subnet);
	}

//bad nodes/DNS initialization
	for(i=0; i<NUM_DNS; i++){
		is_bad_dns[i] = false;
	}
	bad_links = NULL;
#ifdef	MULTI
	pthread_mutex_init(&block_mutex, NULL);
#endif	//MULTI

#ifdef BAD_NODES
	unsigned int num_nodes;
#ifdef MULTI
	struct threads *bad_threads;
#endif //MULTI

	for(num_nodes=0; num_nodes<BAD_NODES; global_id++){
#ifdef MULTI
		bad_threads=new_thread(ATTACKER, global_id, bad_threads, 0);
#endif	//MULTI
#ifndef	MULTI
		threads=new_thread(ATTACKER, global_id, threads, 0);
#endif	//MULTI
		num_nodes++;
	}
#endif //BAD_NODES

/////////////////////////////////////////////////////////////////////
// the main routine	
/////////////////////////////////////////////////////////////////////
// sim_time=1 : 1 second
	for(sim_time = 0; sim_time < SIM_TIME; sim_time++){
#ifdef DEBUG
		fprintf(stderr, "sim_time = %d\n", sim_time);//debug
#endif

#ifdef	DEBUG
		struct killed *killed;
		fprintf(stderr, "dead nodes: ");
		for(killed = dead; killed!=NULL; killed = killed->next){
			fprintf(stderr, "id= %d, ", killed->id);
		}
		fprintf(stderr, "\n");
#endif	//DEBUG
#ifdef	DEBUG
		struct links *links;
		if(bad_links!=NULL){
			fprintf(stderr, "bad_links: ");
			for(links=bad_links; links->prev!=NULL; links=links->prev){}
			for(; links!=NULL; links = links->next){
				fprintf(stderr, "id= %d, ", links->miner_id);
			}
			fprintf(stderr, "\n");
		}
#endif	//DEBUG
// kill/create nodes, manage total hash-rate
		threads = cancel_by_TTL(threads);
//		threads = keep_total_seeds(threads);
		threads = keep_total_nodes(threads);
		if(sim_time==0)
			make_random_connection(threads);
		keep_total_hash_rate_1(threads);
		if(sim_time%600==0){
//			fprintf(stderr, "in bad_links: ");
			
			add_link_records(threads);
#ifdef	MULTI
			add_link_records(bad_threads);
#endif	//MULTI
			print_link_record();
			fprintf(stderr, "SIM_TIME: %d, sim_time: %d\n", SIM_TIME, sim_time);
		}

// routine
#ifdef	MULTI
		pthread_t t1, t2, t3, t4;
		struct thread_arg thread_arg[4];
		int	j;
		for(;threads->next!=NULL; threads=threads->next){}
		for(i=1; threads->prev!=NULL; threads=threads->prev){ i++;}
		thread_arg[0].num 		= i/4;
		thread_arg[0].thread	= threads;
		pthread_create(&t1, NULL, (void * (*)(void *))thread_job, &thread_arg[0]);
		for(j=0; j<=i/4&& threads->next!=NULL; threads=threads->next){ j++;}
		thread_arg[1].num		= i/4;
		thread_arg[1].thread	= threads;
		pthread_create(&t2, NULL, (void * (*)(void *))thread_job, &thread_arg[1]);
		for(j=0; j<=i/4&& threads->next!=NULL; threads=threads->next){ j++;}
		thread_arg[2].num		= i/4;
		thread_arg[2].thread	= threads;
		pthread_create(&t3, NULL, (void * (*)(void *))thread_job, &thread_arg[2]);
		for(j=0; j<=i/4&& threads->next!=NULL; threads=threads->next){ j++;}
		thread_arg[3].num		= 0;
		thread_arg[3].thread	= threads;
		pthread_create(&t4, NULL, (void * (*)(void *))thread_job, &thread_arg[3]);
		pthread_join(t1, NULL);
		pthread_join(t2, NULL);
		pthread_join(t3, NULL);
		pthread_join(t4, NULL);
		for(; threads->prev!=NULL; threads=threads->prev){}
		for(; threads->next!=NULL; threads=threads->next){
			threads->done = false;
		}
		threads->done=false;
#endif
#ifndef	MULTI
		for(;threads->next!=NULL; threads=threads->next){}
		for(;;threads=threads->prev){
#ifdef	DEBUG
		fprintf(stderr, "threads->miner->miner_id= %d\n", threads->miner->miner_id);
#endif	//DEBUG
			if(threads->miner->seed == true){
#ifdef	DEBUG
					fprintf(stderr, "\nminer: %d seed: %d\n", (threads->miner)->miner_id, threads->miner->seed);
					fprintf(stderr, "created at: %d\n", threads->time);
#endif	//DEBUG
					miner_routine(threads->miner);

			}
			else{
				if(threads->type==ATTACKER){
#ifdef	BAD_NODES
					bad_miner_routine(threads->miner);
#endif	//BAD_NODES
				}
				else{
//					if(sim_time>=SIM_TIME-1){
#ifdef	DEBUG
						fprintf(stderr, "\nminer: %d seed: %d\n", (threads->miner)->miner_id, threads->miner->seed);
						fprintf(stderr, "created at: %d\n", threads->time);
#endif	//DEBUG
//					}
					miner_routine(threads->miner);
				}
			}
			if(threads->prev==NULL)
				break;
		}
#endif	//MULTI
		for(i=0; i<SEED_NUM; i++){
#ifdef DEBUG
			fprintf(stderr, "\nminer: %d seed: %d\n", seeds[i].miner_id, seeds[i].seed);
#endif

			miner_routine(seeds[i]);
		}
#ifdef	MULTI
#ifdef	DEBUG
		fprintf(stderr, "bad_threads = %p\n", bad_threads);
#endif	//DEBUG
		for(; bad_threads->next!=NULL; bad_threads=bad_threads->next){
#ifdef	DEBUG
			fprintf(stderr, "bad_threads = %p, bad_threads->next = %p\n", bad_threads, bad_threads->next);
#endif	//DEBUG
			if(bad_threads->next==NULL)
				break;
		}
#ifdef	BAD_NODES
		for(i=0; i<BAD_NODES; i++){
				bad_miner_routine(bad_threads->miner);
				if(bad_threads->prev==NULL)
					break;
				bad_threads=bad_threads->prev;
		}
#endif	//BAD_NODES
#endif	//MULTI
		for(i=0; i<NUM_DNS; i++){
#ifdef DEBUG
			fprintf(stderr, "dns[%d]\n", i);
#endif
//			dns_routine(&dns[i]);
		}
	}
#ifdef DEBUG
	fprintf(stderr, "will cancel_all()\n"); //debug
#endif
	cancel_all(threads);
	cancel_seeds();
	free_killed();
//	free_bad_links();
	print_link_record();
	print_block_record();
	exit(1);
}
#ifdef	MULTI

void *thread_job(struct thread_arg *a){
	int i;
	struct threads *threads;
	threads = a->thread;
	for(i=0;threads!=NULL||(a->num!=0 && i<a->num); threads=threads->next){
		if(threads->done)
			break;
		else
			threads->done=true;
#ifdef	DEBUG
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
		if(threads->next==NULL)
			break;
	}
	return NULL;
}
#endif

#endif	//SIM_ECLIPSE_C
