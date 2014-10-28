#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

#ifndef BLOCK_H
#define	BLOCK_H

struct block {
	struct block*	prev;
	double			height;
	double			time;
	double			miner_id;
	double			size;
	unsigned int	valid;	
};

#endif
