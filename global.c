#ifndef GLOBAL_C
#define GLOBAL_C
#include"params.h"
#include"block.h"
#include"proto-node.h"
#include"connection.h"
struct dns;
struct links;
struct block_record;

unsigned int		sim_time = 0;
bool				is_bad_dns[NUM_DNS];
struct links		*bad_links = NULL;
struct dns 			dns[NUM_DNS];
struct block_record *record = NULL;

#endif

