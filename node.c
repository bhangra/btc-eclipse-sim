#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"thread.c"
#include"connection.c"
#include"action.c"

#ifndef NODE_C
#define NODE_C
void *mining_thread(void *param){

	unsigned int    miner_id;
	double          hashrate;
	struct blocks   *blocks;

	struct links *links, *head, *tail;
	struct link new_comer;

	while(1){
//puzzle solving for block(mining) replaced by Monte Calro simulation
	blocks = mine_block(blocks, miner_id);
/*	for(){
            
	}
*/
	}
}

void *dns_thread(void *my_link){
	while(true){
/*		if(){

		}   
*/
    }
}

void observer(void *param){

}

#endif
