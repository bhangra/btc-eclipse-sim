#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"thread.h"

#ifndef THREAD_C
#define THREAD_C

struct threads *search_head(struct threads *thread){
	struct threads *tmp;
	for(tmp = thread;tmp->prev!=NULL;tmp=tmp->prev){}
	return tmp;
}

//will add variable for threads
struct threads *new_thread(void *thread, struct threads *tail){
	struct threads *tmp;
	tmp 		= malloc(sizeof(struct threads));
	if(tmp == NULL){
		perror("malloc()");
		return NULL;
	}
	if(pthread_create(&tmp->thread, NULL, thread, NULL)!=0){
		perror("pthread_create()");
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

struct threads *cancel_thread(struct threads *will_kill){
	int err;
	struct threads *tmp, *before, *after;
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
	err = pthread_cancel(&will_kill->thread);
	if(err!=0){
		perror("pthread_cancel()");
		strerror(err);
		return NULL;
	}
	if(before==NULL){
		return after;
	}
	else if(after==NULL){
		return before;
	}
	else{
		before->next	= after;
		after->prev		= before;
		return after;
	}
	free(will_kill);
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
