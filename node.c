#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"connection.c"

#ifndef NODE_C
#define NODE_C

void dns_query(){
    
}

void version(){
}
double verack(){
}
//chain_head = add_block(&new_block, &chain_head);
struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	tmp->block			= block;
	chain_head->next 	= tmp;
	tmp->prev 			= chain_head;
	return tmp;
}
void get_block(struct link *link){

}
void request_block(struct block *wanted, struct link *dest){
	
}
int verify_block(struct block *new_block, struct blocks *chain_head){

	struct block *tmp;
	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		return -1;
	}
	else if(new_block->prev != (tmp)){
		return 0;
	}
	return 1;

}
struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct link *from){
	int rerun;
	int validity;
	struct blocks *tmp;
	tmp = chain_head;
	for(rerun = 1; rerun==1;){
		rerun = 0;
		for(;;){
			validity = verify_block(block, tmp);
			if(validity==1){
				add_block(block, chain_head);
			}
			else if(validity==0){
				request_block(block->prev, from);
				tmp = tmp->prev;
			}
			else if(validity==-1){
				return chain_head;
			}
		}
		
	}
}

int process_msg(struct msg_hdr *msg_ptr){
	const struct msg_hdr *hdr;
	hdr = msg_ptr;
	if(strncmp(hdr->command, "addblock", 12)){
		
	}
	else if(strncmp(hdr->command, "block", 12)){
		
	}
	else if(strncmp(hdr->command, "newhead", 12)){

	}
	else if(strncmp(hdr->command, "getblock", 12)){

	}
	else if(strncmp(hdr->command, "roundrobin", 12)){

	}
	else if(strncmp(hdr->command, "getaddr", 12)){
		
	}
	else if(strncmp(hdr->command, "addr", 12)){

	}
	else if(strncmp(hdr->command, "version", 12)){
		
	}
	else if(strncmp(hdr->command, "verack", 12)){

	}
}

void *mining_thread(void *param){
/*
	unsigned int	miner_id;
	double 			hashrate;
	struct block	*chain_head, *new;
	struct blocks	current, *head, tail;
*/
	struct links links, *head, *tail;
	struct link	new_comer;
	double x, y;
	int mined;
	int times;
	
	links.link 	= &new_comer;
	head		= &links;
	tail		= &links;
	while(1){
//puzzle solving for block(mining) replaced by Monte Calro simulation
		for(times = 0;times < 1000;times++){
			x = rand()/(RAND_MAX);
			y = rand()/(RAND_MAX);
			if((x*x+y*y)>0.5){
				mined = 1;
				printf("mined block in %d times\n", times);
/*
				head = malloc(sizeof(struct block));
				head->prev 		= chain_head;
				head->height	= chain_head->height++;
				head->time		= ~~~
				head->miner_id	= miner_id;
				head->size		= sizeof(struct block);
				head->valid		= true;
*/
				break;
			}
/*			for(){

			}
*/
		}
		
	}
}

void *dns_thread(void *param){
	while(true){
		
	}
}

#endif
