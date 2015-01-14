#ifndef PARAMS_H
#define PARAMS_H

//#define MULTI
#define MALLOC_CHECK_ 3
//#define DEBUG
//#define MEM_DEBUG
//#define ADDR_DEBUG
#define ASSERT

//#define BEHAVIOR
#define NUM_DNS		6
#define GROUPS		2
#define BAD_DNS		0
#define NUM_GROUPS	2
#define ATTACKER	0
#define HONEST		1
#define BUF_SIZE	10000//10000
#define THOUSAND	100//0
#define N_MAX_CONNECTIONS 125
#define MAX_OUTBOUND_CONNECTIONS 8


#define SEED_NUM	4
#define	TOTAL_NODES	30
#define BAD_NODES	3
//(1000nodes+5bad) * 12h = 9min
#define NOT_NAT /*for every*/ 5//11 //+1th
#define AVE_TTL		60*60*2
//#define AVE_TTL		1000
#define SEED_TTL	60*60*24*1
//#define SEED_TTL	2000
#define SIM_DAYS	2//30
//#define SIM_TIME	60*60*24*SIM_DAYS
#define SIM_TIME	60*60*24
//1000nodes*12hour = 2min15~30s
//24,000nodes*30day = 702min &killed

//#define INIT_NODES	TOTAL_NODES



#endif
