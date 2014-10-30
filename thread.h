#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#ifndef THREAD_H
#define THREAD_H

//change thread to struct 


struct threads{
	struct threads	*prev;
//	pthread_t		thread;
	struct miner	*miner;
	int				type;
	struct threads	*next;
//will add TTL and make main thread kill it
};

#endif
