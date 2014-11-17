#ifndef BAD_ROUTINE_C
#define BAD_ROUTINE_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"action.c"

void bad_dns_routine(struct dns *dns){
	fprintf(stderr, "\n");
	for(; (&dns->new_comer)->num_msg!=0;){
		fprintf(stderr, "num of msgs to dns: %d\n", (&dns->new_comer)->num_msg);
		read_msg(&dns->new_comer);
		fprintf(stderr, "read msg to dns\n");
//		dns->seeds = process_bad_dns(&dns->new_comer, /*(struct links*)&*/dns->seeds);
	}
}

void bad_miner_routine(struct miner *miner){
	int				i;
	struct links	*links;
	struct link		*link;
	struct blocks	*blocks;
	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
		miner->seed = false;
	}
	else if(miner->boot == true){
		dns_query(&dns[rand()%5], &miner->new_comer, miner->miner_id);
		miner->boot = false;
	}
	else{
		for(link=&miner->new_comer; link->num_msg!=0;){
			fprintf(stderr, "new_comer link\n"); //debug
			read_msg(link);
			miner->links = process_new(&miner->new_comer, miner);
		}
		links = miner->links;
		if(links==NULL){
			i=0;
			miner->boot=true;
		}
		else{
			for(;links->prev!=NULL; links=links->prev){}
			for(i=0/*debug*/; links!=NULL; links=links->next){
				for(link=links->link; link->num_msg!=0;){
					read_msg(link);
					process_msg(&miner->new_comer, links, miner);
				}
				i++;
				if(links->next==NULL)
					break;
			}
		}
	}
	if(miner->links!=NULL){
//		if(i<LEAST_NEIGHBOR && miner->links!=NULL && ((miner->links)->link)->dest!=(miner->links)->new_comer){
//			getaddr((miner->links)->link, miner->miner_id);
//		}
	}
}
#endif
