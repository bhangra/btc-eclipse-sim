#ifndef BLOCK_H
#define BLOCK_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

struct block {
	struct block	*prev;
	double			height;
	double			time;
	unsigned int	miner_id;
	unsigned int	size;
	unsigned int	valid;	
};

struct blocks{
	struct blocks	*prev;
	struct block	*block;
	struct blocks	*next;
};

#endif
