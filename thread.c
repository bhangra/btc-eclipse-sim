#ifndef THREAD_C
#define THREAD_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"proto-node.h"
#include"thread.h"
#include"connection.c"


struct threads *search_head(struct threads *thread){
	struct threads *tmp;
	for(tmp = thread;tmp->prev!=NULL;tmp=tmp->prev){}
	return tmp;
}
/*
struct threads *bad_new_thread(struct threads *threads){
	struct threads *bad, *new;
	struct  miner 	*miner;
	new 		= malloc(sizeof(struct threads));
	if(new == NULL){
		perror("malloc()");
		return NULL;
	}
	memset(new, 0, sizeof(struct threads));
	new->type = ATTACKER;	//distinguish miner/attacker
	new->time = sim_time; //debug
	if((new->miner=malloc(sizeof(struct miner)))==NULL){
		perror("malloc()");
		free(new);
		new=NULL;
		return NULL;
	}
	miner=new->miner;
	memset(miner, 0, sizeof(struct miner));
	miner->TTL	= SIM_TIME;
	miner->one_way	= NOT_NAT; 
	miner->miner_id = global_id;
	miner->max		= TOTAL_NODES; 
	miner->least	= TOTAL_NODES;
	miner->neighbor = 0;
	miner->seed		= 0;//rand()%2;
	miner->boot		= true;
	miner->links	= NULL;
	miner->blocks	= NULL;
	miner->new_chain= NULL;
	miner->hash_rate= 0;
	
	if(bad_threads!=NULL){
		for(bad=bad_threads; bad->next!=NULL; bad=bad->next){}
		bad->next=bad;		
	}
	else
		bad_threads = new;
	return new;
}*/

//will add variable for threads
struct threads *new_thread(int type, unsigned int miner_id,  struct threads *threads, unsigned int seed){
	struct miner	*miner;
	struct threads	*new, *tmp=NULL;//, *bad;
	new 		= malloc(sizeof(struct threads));
/*	if(new == NULL){
		perror("malloc()");
		return NULL;
	}
*/	memset(new, 0, sizeof(struct threads));
	new->type = type;	//distinguish miner/attacker

	new->time = sim_time; //debug
/*	if((*/new->miner=malloc(sizeof(struct miner));/*)==NULL){*/
//		perror("malloc()");
//		free(new);
//		new=NULL;
//		return NULL;
//	}
	miner=new->miner;
	memset(miner, 0, sizeof(struct miner));
	//times, TTL = 1 : 1 second
	miner->TTL	= (unsigned int)sim_time+((rand()%(AVE_TTL*2)));
	if(seed==true)
		miner->TTL = (unsigned int)sim_time+((rand()%(SEED_TTL*2)));
	if(new->type==ATTACKER)
		miner->TTL = SIM_TIME;
//	fprintf(stderr, "miner->TTL = %d\n", miner->TTL);
//	miner->group	= rand()%2;	

//represents node under NAT, which stops connection initiated by others
	miner->one_way	= rand()%(NOT_NAT+1);
	if((seed==true)||new->type==ATTACKER)
		miner->one_way=NOT_NAT;
	miner->miner_id = miner_id;
//	miner->neighbor = 0;
	miner->n_addrman= 0;
	miner->n_outbound=0;
	miner->n_inbound= 0;
	miner->seed		= seed;//rand()%2;
	miner->boot		= true;
	miner->subnet	= rand()&0xffff0000;
//	miner->links	= NULL;
	memset(&miner->addrman, 0, sizeof(struct addrman));
	miner->addrman.caddrinfo = NULL;
	miner->outbound	= NULL;
	miner->inbound	= NULL;
	miner->blocks	= NULL;
	miner->new_chain= NULL;
	miner->hash_rate= (double)rand() / (RAND_MAX);
	miner->connect_count = 1;
	if(new->type==ATTACKER)
		miner->hash_rate=0;
	if(threads!=NULL){
		tmp = threads;
		for(;tmp->next!=NULL; tmp=tmp->next){
			if(tmp->next==NULL){
				break;
			}
		}
		new->next = NULL;
		new->prev = tmp;
		tmp->next = new;
	}
	else{
		new->next = NULL;
		new->prev = NULL;
	}
	if(new->type==ATTACKER){
		struct bad_threads *bad;
		if(bad_threads!=NULL){
			for(bad=bad_threads; bad->next!=NULL; bad=bad->next){}
			bad->next=malloc(sizeof(struct bad_threads));//new;		
			bad->next->prev=bad;
			bad=bad->next;
			bad->miner = new->miner;
			bad->next=NULL;
			bad->thread = new;
			
		}
		else{
			bad = malloc(sizeof(struct bad_threads));
			bad->miner = new->miner;
			bad->next = NULL;// new;
			bad->prev = NULL;
			bad->thread = new;
			bad_threads=bad;
		}
	}

	return new;
}
struct threads *keep_total_seeds(struct threads *threads){
#ifndef SEED_NUM
#define SEED_NUM 10
#endif
	unsigned int seed;
	unsigned int current;
	struct threads *tmp;
	current = 0;
	seed 	= true;
	if(threads!=NULL){
		for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if((tmp->miner)->seed == true){
				current++;
			}
		}
	}
	for(tmp=threads;current<=SEED_NUM; current++){
		global_id++;
		tmp = new_thread(HONEST, global_id, tmp, seed);
#ifdef DEBUG
		fprintf(stderr, "added seed node\n");
#endif
	}	
	return tmp;	
}
struct threads *keep_total_nodes(struct threads *threads){
	unsigned int	current=0, seed=false;

	struct threads	*tmp;
	if(threads!=NULL){
		for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			current++;
		}
	}
#ifndef	MULTI
#ifdef	BAD_NODES
	for(tmp=threads; current<=TOTAL_NODES+BAD_NODES; current++){
#endif	//BAD_NODES
#ifndef	BAD_NODES
	for(tmp=threads; current<=TOTAL_NODES; current++){
#endif	//BAD_NODES
#endif	//MULTI
#ifdef	MULTI
	for(tmp=threads; current<=TOTAL_NODES; current++){
#endif
		global_id++;
		tmp = new_thread(HONEST, global_id, tmp, seed);
	}
	return tmp;
}

void free_blocks(struct blocks *blocks, struct blocks *meblocks){
	struct blocks    *tmp, *tmp2;
/*	if(blocks==meblocks){
		return;
	}*/
	if(blocks==NULL)
		return;
	for(tmp=blocks; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL;){
		free(tmp->block);
		tmp->block = NULL;
		tmp2=tmp->next;
//		if(tmp!=meblocks)
		free(tmp);
		tmp=NULL;
		tmp=tmp2;
	} 
}
void free_node_s_links(struct links *melinks){
	struct links	*tmp, *tmp2;
//	if((links=melinks)){
//		return;
//	}
	if(melinks==NULL)
		return;
	for(tmp=melinks; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL; ){
		free(tmp->link);
		tmp->link=NULL;
		tmp2=tmp->next;
//		if(tmp!=melinks)
		free(tmp);
		tmp=NULL;
		tmp=tmp2;
	}	
}
void free_node_s_caddrinfo(struct caddrinfo *melinks){
	struct caddrinfo	*tmp, *tmp2;
//	if((links=melinks)){
//		return;
//	}
	if(melinks==NULL)
		return;
	for(tmp=melinks; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL; ){
//		free(tmp->link);
//		tmp->link=NULL;
		tmp2=tmp->next;
//		if(tmp!=melinks)
		free(tmp);
		tmp=NULL;
		tmp=tmp2;
	}	
}

//for cancel_all() and cancel_by_TTL
struct threads *cancel_thread(struct threads *will_kill){
	struct miner	*tmp;
	struct threads *before, *after, *ret;
	if(will_kill==NULL){
		return NULL;
	}
	before	= will_kill->prev;
	after	= will_kill->next;

	tmp = will_kill->miner;
	free_node_s_caddrinfo(tmp->addrman.caddrinfo);
	free_node_s_links((struct links*)tmp->outbound);
	free_node_s_links((struct links*)tmp->inbound);
	free_blocks(tmp->blocks, (struct blocks*)&tmp->blocks);
	free_blocks(tmp->new_chain, (struct blocks*)&tmp->new_chain);
	free_from_bad_links(will_kill->miner->miner_id, &will_kill->miner->new_comer);
	free(will_kill->miner);
	will_kill->miner=NULL;
	free(will_kill);
	will_kill=NULL;
	if(before!=NULL && after!=NULL){
		before->next	= after;
		after->prev		= before;
		ret =  after;
	}
	else if(before!=NULL){
		before->next	= NULL;
		ret =  before;
		}
	else if(after!=NULL){
		after->prev	= NULL;
		ret =  after;
	}
	else{
		ret = NULL;
	}
//	fprintf(stderr, "cancel_thread returned: %p\n", ret);//debug
	return ret;
}

//for cancel_by_TTL
struct threads *cancel_one_thread(struct threads *will_kill){
	struct killed *killed, *tmp;
//	struct links *links, *prev=NULL, *next=NULL, *after;
	killed = malloc(sizeof(struct killed));
	killed->next	= NULL;
	killed->id		= (will_kill->miner)->miner_id;
#ifdef DEBUG
	fprintf(stderr, "dead updated\n");
#endif

	if(dead==NULL)
		dead = killed;
	else{
		for(tmp=dead; tmp->next!=NULL; tmp=tmp->next){}
		tmp->next=killed;
	}
	free_links(will_kill);
	free_dns_rec(will_kill);
	return cancel_thread(will_kill);
}

struct threads *cancel_by_TTL(struct threads *list){
	struct threads *thread;
	if(list==NULL)
		return list;
	for(thread = list; thread->next!=NULL; thread=thread->next){}
	for(;thread!=NULL; thread=thread->prev){
		if((thread->miner)->TTL <= sim_time){
#ifdef DEBUG
			fprintf(stderr, "will kill %d\n", thread->miner->miner_id);
#endif
			thread=cancel_one_thread(thread);
		}
		if(thread->prev==NULL)
			break;
	}
	return thread;
}

void cancel_all(struct threads *head){
	struct threads *tmp, *tmp2;
	if(head->prev!=NULL){
		tmp = search_head(head);
	}else{
		tmp = head;
	}
	for(;tmp!=NULL;){
		tmp2 = tmp;
		tmp = cancel_thread(tmp);
		if(tmp==tmp2)
			break;
	}
}
void cancel_seeds(){
	int i;
	for(i=0; i<SEED_NUM; i++){
		free_node_s_caddrinfo(seeds[i]->addrman.caddrinfo);
		free_node_s_links((struct links*)seeds[i]->outbound);
		free_node_s_links((struct links*)seeds[i]->inbound);
		free_blocks(seeds[i]->blocks, (struct blocks*)&seeds[i]->blocks);
		free_blocks(seeds[i]->new_chain, (struct blocks*)&seeds[i]->new_chain);
		free_from_bad_links(seeds[1]->miner_id, &seeds[i]->new_comer);
		free(seeds[i]);
	}
}

void free_killed(){
	struct killed *killed, *next;
	for(killed=dead; killed!=NULL; killed=next){
		next=killed->next;
		free(killed);
	}
}

void keep_total_hash_rate_1(struct threads *threads){
	struct threads	*tmp;
	double 			sum;
	sum = 0;
	for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		sum += (tmp->miner)->hash_rate;
	}
	for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		(tmp->miner)->hash_rate = (tmp->miner)->hash_rate/sum;
//		fprintf(stdout, "hash_rate = %f\n", (tmp->miner)->hash_rate);
	}
}

#endif
