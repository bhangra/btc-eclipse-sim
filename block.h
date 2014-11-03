#ifndef BLOCK_H
#define BLOCK_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<openssl/sha.h>

struct block {
//	struct block	*prev;
	unsigned char	prev[SHA256_DIGEST_LENGTH];
	unsigned int	height;
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
