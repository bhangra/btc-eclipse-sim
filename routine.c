#ifndef ROUTINE_C
#define ROUTINE_C


#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"action.c"
//#include"proto-node.h"

void dns_routine(struct dns *dns){
	for(; (&dns->new_comer)->num_msg!=0;){
		fprintf(stderr, "num of msg to dns: %d\n", (&dns->new_comer)->num_msg); //debug
		read_msg(&dns->new_comer);
		fprintf(stderr, "have read msg\n"); //debug
		dns->seeds = process_dns(&dns->new_comer, dns->seeds);
		fprintf(stderr, "processed msg\n");  //debug
	}
}

void miner_routine(struct miner *miner){
	int 			i;
	struct links 	*links;
	struct link		*link;
	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			fprintf(stderr, "will send dns_seed request\n");//debug
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
			fprintf(stderr, "sent dns_seed request\n");//debug
		}
		for(i=0; i<rand()%6; i++){}
		dns_query(&dns[i], &miner->new_comer);
		fprintf(stderr, "sent dns_query\n");
		miner->boot = false;
	}
	if(miner->boot == true){
		for(i=0; i<rand()%6; i++){}
		dns_query(&dns[i], &miner->new_comer);
		miner->boot = false;
	}
	else{
		for(links=miner->links; links!=NULL; links=links->next){
			for(link=links->link; link->num_msg!=0;){
				read_msg(link);	
				process_msg(&miner->new_comer, links, miner);
			} 
		}
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id, miner);
}
void links_kill(int miner_id, struct threads *threads){
	struct threads	*tmp;
	struct miner	*miner;
	struct links	*links, *after, *before;
	for(tmp = threads; tmp->prev==NULL;tmp=tmp->prev){}
	for(miner=tmp->miner; tmp!=NULL; tmp=tmp->next){
		for(links=miner->links; links!=NULL; links=links->next){
			if(links->miner_id==miner_id){
				remove_links(links);
			}
		}
	}
}
void miner_kill(struct miner *miner){
	
}

#endif
