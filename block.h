#ifndef BLOCK_H
#define BLOCK_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<openssl/sha.h>

struct block {
//	struct block	*prev;
	unsigned char	hash[SHA256_DIGEST_LENGTH];
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

struct block_record{
	struct block_record	*next;
	struct block_record	*same;
//	unsigned char		hash[SHA256_DIGEST_LENGTH];
	unsigned int		mined_time;
	unsigned int		height;
	unsigned int		miner_id;
	double				hash_rate;
	unsigned int		num_nodes;
};


#endif
