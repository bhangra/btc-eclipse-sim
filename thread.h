#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#ifndef THREAD_H
#define THREAD_H

struct threads{
	struct threads	*prev;
	pthread_t		thread;
	struct threads	*next;
//will add TTL and make main thread kill it
};

#endif
