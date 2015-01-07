#ifndef CONNECTION_H
#define CONNECTION_H

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>

#include"protocol.h"
#include"params.h"

#define MAX_SIZE	100
//#define BUF_SIZE	10000

struct link{
	pthread_mutex_t	rcv_mutex;
    unsigned int	num_msg; 	//number of messages unprocessed 	
    unsigned int   	read_pos;
	unsigned int	write_pos;
	unsigned char	buf[BUF_SIZE];
	unsigned char	sbuf[BUF_SIZE];
	unsigned char	process_buf[BUF_SIZE];
	struct link		*dest;
	bool			fgetblock;
};

struct links{
	unsigned int		group;
	unsigned int		miner_id;
	unsigned int		subnet;
	unsigned int		n_time;
	struct link			*new_comer;
	struct link			*link;
	struct links		*prev;
	struct links		*next;
};


#endif
