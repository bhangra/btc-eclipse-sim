#ifndef PARAMS_H
#define PARAMS_H

#define MALLOC_CHECK_ 3
//#define DEBUG
//#define MEM_DEBUG

#define NUM_DNS		6
#define GROUPS		2
#define BAD_DNS		0
#define NUM_GROUPS	2
#define ATTACKER	0
#define HONEST		1


#define SEED_NUM	5
#define	TOTAL_NODES	1000
//#define BAD_NODES	10
#define NOT_NAT /*for every*/ 10 //+1th

#define SIM_DAYS	30
//#define SIM_TIME	60*60*24*SIM_DAYS
#define SIM_TIME	60*60*12 //1000nodes*12hour = 3min
//24,000nodes*30day = 702min &killed
//2400nodes*30day =40min
#define AVE_TTL		60*60*12
//#define AVE_TTL		1000
#define SEED_TTL	60*60*24*5

//#define INIT_NODES	TOTAL_NODES



#endif
