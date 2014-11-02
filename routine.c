#ifndef ROUTINE_C
#define ROUTINE_C


#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"action.c"
//#include"proto-node.h"


void miner_routine(struct miner *miner){
	struct links 	*links;
	struct link		*link;
	for(links=miner->links; links!=NULL; links=links->next){
		for(link=links->link; link->num_msg!=0;){
			read_msg(link);
			process_msg(links);
		} 
	}
	fprintf(stderr, "after msg\n");//debug
	miner->blocks = mine_block(miner->blocks, miner->miner_id);
}

void miner_kill(struct miner *miner){

}

#endif
