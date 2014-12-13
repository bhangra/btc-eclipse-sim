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

//will add variable for threads
struct threads *new_thread(int type, unsigned int miner_id,  struct threads *threads, unsigned int seed, unsigned int now){
	struct miner	*miner;
	struct threads	*new, *tmp=NULL;
	new 		= malloc(sizeof(struct threads));
	if(new == NULL){
		perror("malloc()");
		return NULL;
	}
	memset(new, 0, sizeof(struct threads));
	new->type = type;	//distinguish miner/attacker
	if((new->miner=malloc(sizeof(struct miner)))==NULL){
		perror("malloc()");
		free(new);
		return NULL;
	}
	miner=new->miner;
	memset(miner, 0, sizeof(struct miner));
	//times, TTL = 1 : 1 second
	if(now>=0){
		miner->TTL	= (unsigned int)sim_time+((rand()%(AVE_TTL*2)));
	}
	fprintf(stderr, "miner->TTL = %d\n", miner->TTL);
	miner->miner_id = miner_id;
	miner->max		= 1+rand()%5;//will fix 
	miner->least	= rand()%5;//will fix
	miner->neighbor = 0;
	miner->seed		= seed;//rand()%2;
	miner->boot		= true;
	miner->links	= NULL;
	miner->blocks	= NULL;
	miner->hash_rate= (double)rand() / (RAND_MAX);
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
	return new;
}

void keep_total_nodes(struct threads *threads){
	unsigned int	current=0;
	struct threads	*tmp;
	for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		current++;
	}
	tmp=threads;
	for(; current<TOTAL_NODES; current++){
		miner_id++;
		tmp = new_thread(1, miner_id, tmp, 1, sim_time);
	}
}

void free_blocks(struct blocks *blocks, struct blocks *meblocks){
	struct blocks    *tmp, *tmp2;
	if(blocks==meblocks){
		return;
	}
	if(blocks==NULL)
		return;
	for(tmp=blocks; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL;){
		free(tmp->block);
		tmp2=tmp->next;
		if(tmp!=meblocks)
			free(tmp);
		tmp=tmp2;
	} 
}
void free_node_s_links(struct links *links, struct links *melinks){
	struct links	*tmp, *tmp2;
	if(links=melinks){
		return;
	}
	if(tmp==NULL)
		return;
	for(tmp=links; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL; ){
		free(tmp->link);
		tmp2=tmp->next;
		if(tmp!=melinks)
			free(tmp);
		tmp=tmp2;
	}	
}


//for cancel_all()
struct threads *cancel_thread(struct threads *will_kill){
	struct miner	*tmp;
	struct threads *before, *after, *killed;
	if(will_kill==NULL){
		return NULL;
	}
	before	= will_kill->prev;
	after	= will_kill->next;

	tmp = will_kill->miner;
	free_node_s_links(tmp->links, (struct links*)&tmp->links);
	free_blocks(tmp->blocks, (struct blocks*)&tmp->blocks);
	free(will_kill->miner);
	free(will_kill);
	killed = will_kill;
	if(before!=NULL && after!=NULL){
		before->next	= after;
		after->prev		= before;
		return after;
	}
	else if(before!=NULL){
		before->next	= NULL;
		return before;
	}
	else if(after!=NULL){
		after->prev	= NULL;
		return after;
	}
	else{
		return NULL;
	}
}

//for cancel_by_TTL
struct threads *cancel_one_thread(struct threads *will_kill){
	free_links(will_kill);
	return cancel_thread(will_kill);
}

struct threads *cancel_by_TTL(unsigned int now, struct threads *list){
	struct threads *thread;
	for(thread = list; thread->next!=NULL; thread=thread->next){}
	for(;thread!=NULL; thread=thread->prev){
		if((thread->miner)->TTL <= now){
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

void keep_total_hash_rate_1(struct threads *threads){
	struct threads	*tmp;
	double 			sum;
	sum = 0;
	for(tmp=threads; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL; tmp=tmp->next){
		sum += (tmp->miner)->hash_rate;
	}
	for(tmp=threads; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL; tmp=tmp->next){
		(tmp->miner)->hash_rate = (tmp->miner)->hash_rate/sum;
//		fprintf(stdout, "hash_rate = %f\n", (tmp->miner)->hash_rate);
	}
}

#endif
