#ifndef PARAMS_H
#define PARAMS_H

//#define MULTI
#define MALLOC_CHECK_ 3
//#define VER_DEBUG
//#define DEBUG
//#define BLOCK_DEBUG
//#define MEM_DEBUG
//#define ADDR_DEBUG
#define ASSERT

//#define BEHAVIOR //stops seed from handling blocks
//#define NONSENSE_ADDR //make bad nodes send non-existent network address
#define NUM_DNS		6
#define GROUPS		2
#define BAD_DNS		0
#define NUM_GROUPS	2
#define ATTACKER	0
#define HONEST		1
#define BUF_SIZE	10000//2500//10000
#define THOUSAND	10//00
#define N_MAX_CONNECTIONS 125
#define MAX_OUTBOUND_CONNECTIONS 4//8

//s6t100b16n11a6d30
#define SEED_NUM	6
<<<<<<< HEAD
#define	TOTAL_NODES	100
#define BAD_NODES	16
=======
#define	TOTAL_NODES	50
#define BAD_NODES	8
>>>>>>> b9eaffcf71c47407e974238fd0ec85221481324e
//(1000nodes+5bad) * 12h = 9min
#define NOT_NAT /*for every*/ 11 //+1th
#define AVE_TTL		60*60*6
//#define AVE_TTL		1000
#define SEED_TTL	60*60*24*1
//#define SEED_TTL	2000
#define SIM_DAYS	30
#define SIM_TIME	60*60*24*SIM_DAYS
//#define SIM_TIME	60*60*24
//1000nodes*12hour = 2min15~30s
//24,000nodes*30day = 702min &killed

//#define INIT_NODES	TOTAL_NODES



#endif
