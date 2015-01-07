#ifndef ADDRMAN_H
#define ADDRMAN_H

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<time.h>
#include<math.h>

#define ADDRMAN_BUCKET_SIZE			64
#define ADDRMAN_TRIED_BUCKET_COUNT	64
#define ADDRMAN_NEW_BUCKET_COUNT	64
#define ADDRMAN_NEW_BUCKETS_PER_ADDRESS 4
#define ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP 32
#define ADDRMAN_TRIED_BUCKETS_PER_GROUP 4
#define ADDRMAN_TRIED_ENTRIES_INSPECT_ON_EVICT 4
#define ADDRMAN_GETADDR_MAX_PCT		23
#define ADDRMAN_GETADDR_MAX			2500
#define ADDRMAN_MIN_FAIL_DAYS		7
#define ADDRMAN_MAX_FAILURES		10
#define	ADDRMAN_RETRIES				3
#define ADDRMAN_HORIZON_DAYS		30

struct caddrinfo{
	struct caddrinfo	*next;
	struct caddrinfo	*prev;
	struct link			*new_comer;
	unsigned int		subnet;
	struct link			*source; //will fix
	unsigned int		miner_id;
	unsigned int		nid;
	unsigned int		n_last_success;
	unsigned int		n_attempts;
	unsigned int		n_ref_count;
	bool				f_in_tried;
	unsigned int		n_random_pos;
	unsigned int		n_last_try;
	unsigned int		n_time;
};

struct addrman{
	char				*n_key;
	unsigned int		n_tries;
//	struct caddrinfo	*v_random;
	struct caddrinfo	*caddrinfo;
	unsigned int		v_random[ADDRMAN_NEW_BUCKET_COUNT+ADDRMAN_TRIED_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE];
	unsigned int		v_random_size;
	unsigned int		n_id_count;
	unsigned int		n_tried;
	unsigned int		n_new;
	unsigned int		vv_new[ADDRMAN_NEW_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE];
	unsigned int		vv_tried[ADDRMAN_TRIED_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE];	
};

#endif
