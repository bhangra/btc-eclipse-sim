#ifndef PROTO_NODE_H
#define PROTO_NODE_H

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"connection.h"

struct miner{
	unsigned int	TTL;
	unsigned int	group; 	//for attacker to classify nodes
	bool            boot;
	bool            seed;
	bool            one_way;

	unsigned int	miner_id;
	double			hash_rate;
	struct blocks	*blocks;
	struct blocks	*new_chain;
	struct links	*links;
	struct link		new_comer;
};

struct dns{
	struct links	*seeds;
	struct link		new_comer;
};

struct dns dns[5];

#endif
