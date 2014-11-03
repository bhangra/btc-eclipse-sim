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



void miner_routine(struct miner *miner){
	int 			i;
	struct links 	*links;
	struct link		*link;
	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			dns_seed(&dns[i], &miner->new_comer);
		}
	}
/*	else if(){

	}
*/	else{
		for(links=miner->links; links!=NULL; links=links->next){
			for(link=links->link; link->num_msg!=0;){
				read_msg(link);	
				process_msg(links);
			} 
		}
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id);
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
