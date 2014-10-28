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
struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	chain_head->next 	= tmp;
	tmp->prev 			= chain_head;
	return tmp;
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
