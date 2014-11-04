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


struct threads *search_head(struct threads *thread){
	struct threads *tmp;
	for(tmp = thread;tmp->prev!=NULL;tmp=tmp->prev){}
	return tmp;
}

//will add variable for threads
struct threads *new_thread(int type, unsigned int miner_id,  struct threads *threads){
	struct miner	*miner;
	struct threads	*new, *tmp;
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
	miner->miner_id = miner_id;
	miner->seed		= rand()%2;
	miner->boot		= true;
	miner->links	= NULL;
	miner->blocks	= NULL;
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
void free_blocks(struct blocks *blocks){
	struct blocks    *tmp;
	for(tmp=blocks; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL;){
		free(tmp->block);
		tmp=tmp->next;
		free(tmp->prev);
	} 
}
void free_links(struct links *links){
	struct links	*tmp;
	for(tmp=links; tmp->prev!=NULL; tmp=tmp->prev){}
	for(; tmp!=NULL;){
		free(tmp->link);
		tmp=tmp->next;
		free(tmp->prev);
	}	
}
struct threads *cancel_thread(struct threads *will_kill){
	struct miner	*tmp;
	struct threads *before, *after;
	if(will_kill==NULL){
		return NULL;
	}
	if(will_kill->prev != NULL){
		before	= will_kill->prev;
	}else{
		before 	= NULL;
	}
	if(will_kill->next != NULL){
		after	= will_kill->next;
	}else{
		after	= NULL;
	}
	tmp = will_kill->miner;
	free_links(tmp->links);
	free_blocks(tmp->blocks);
	free(will_kill->miner);
	free(will_kill);
	if(before==NULL){
		return after;
	}
	else if(after==NULL){
		return NULL;
	}
	else{
		before->next	= after;
		after->prev		= before;
		return after;
	}
}

void cancel_all(struct threads *head){
	struct threads *tmp;
	if(head->prev!=NULL){
		tmp = search_head(head);
	}else{
		tmp = head;
	}
	for(;tmp!=NULL;){
		tmp = cancel_thread(tmp);
	}
}


#endif
