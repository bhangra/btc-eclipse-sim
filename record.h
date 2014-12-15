#ifndef record_H
#define record_H

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<openssl/sha.h>

#include"block.h"
#include"connection.h"

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

struct record_printer{
	struct block_record *record;
	struct record_printer *same;

};

struct link_record{
	struct link_record	*next;
	unsigned int		dest_group;
	unsigned int		dest_id;
};
struct node_record{
	unsigned int		my_id;
	unsigned int		my_group;
	struct link_record	*record;
	struct node_record	*next;
	struct node_record	*prev;
	struct node_record	*same;
};

#endif
