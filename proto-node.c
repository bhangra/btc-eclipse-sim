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

#ifndef PROTO-NODE_C
#define PROTO-NODE_C

struct miner{
	unsigned int	group; //for attacker to classify nodes
	unsigned int	miner_id;
	double			hash_rate;
	struct blocks	*blocks;
	struct links	*links;
	struct link		new_comer;
};

struct dns{
	struct links	*seeds;
	struct link		new_comer;
};


#endif
