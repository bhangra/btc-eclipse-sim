#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>

#include"protocol.h"

#ifndef CONNECTION_H
#define CONNECTION_H

#define MAX_SIZE	100
#define BUF_SIZE	10000

struct link{
	pthread_mutex_t	rcv_mutex;
    unsigned int	num_msg; 	//number of messages unprocessed 	
    unsigned int   	read_pos;
	unsigned int	write_pos;
	char			buf[BUF_SIZE];
	char			sbuf[BUF_SIZE];
	char			process_buf[BUF_SIZE];
	struct link		*dest;
};

struct links{
	struct links		*prev;
	struct link			*link;
	struct links		*next;
};

#endif
