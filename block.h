#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

//the "selfish mining" researcher used Monte Carlo simulator

struct block {
	struct block*	prev;
	double			height;
	double			time;
	double			miner_id;
	double			size;
	unsigned int	valid;	
};
