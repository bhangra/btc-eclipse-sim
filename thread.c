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
struct threads *new_thread(int type, struct threads *tail){
	struct threads *tmp;
	tmp 		= malloc(sizeof(struct threads));
	if(tmp == NULL){
		perror("malloc()");
		return NULL;
	}
	tmp->type = type;
	if((tmp->miner=malloc(sizeof(struct miner)))==NULL){
		perror("malloc()");
		free(tmp);
		return NULL;
	}
	if(tail!=NULL){
		tail->next	= tmp;
		tmp->prev	= tail;
		tmp->next	= NULL;
	}
	else{
		tmp->prev	= NULL;
		tmp->next 	= NULL;
	}
	return tmp;
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
