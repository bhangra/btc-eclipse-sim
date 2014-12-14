#ifndef ROUTINE_C
#define ROUTINE_C


#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"action.c"
//#include"proto-node.h"

#define LEAST_NEIGHBOR 8

void dns_routine(struct dns *dns){
//	fprintf(stderr, "\n");
	for(; (&dns->new_comer)->num_msg!=0;){
//		fprintf(stderr, "num of msgs to dns: %d\n", (&dns->new_comer)->num_msg);
		read_msg(&dns->new_comer);
//		fprintf(stderr, "read msg to dns\n");
		dns->seeds = process_dns(&dns->new_comer, /*(struct links*)&*/dns->seeds);
	}
}

void miner_routine(struct miner *miner){
	int 			i, n;
	struct links 	*links;
	struct link		*link;
	struct blocks 	*blocks;
//	fprintf(stderr, "entered miner_routine()\n"); //debug
	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
//		for(i=0; i<rand()%6; i++){}
//		dns_query(&dns[i], &miner->new_comer, miner->miner_id);
//		fprintf(stderr, "sent dns_query\n");
//		miner->boot = false;
		miner->seed = false;
	}
	else if(miner->boot == true){
//		for(i=0; i<rand()%6; i++){}
		dns_query(&dns[rand()%5], &miner->new_comer, miner->miner_id);
//		fprintf(stderr, "sent dns_query\n");
		miner->boot = false;
	}
	else{
		for(link=&miner->new_comer; link->num_msg!=0;){
//			fprintf(stderr, "new_comer link\n"); //debug
			read_msg(link);
			miner->links = process_new(&miner->new_comer, miner);
//			fprintf(stderr, "miner->links = %p\n", miner->links);
		}
		links = miner->links;
		if(links==NULL){
			i=0;
			miner->boot=true;
		}
//			return;
		else{
			for(;links->prev!=NULL; links=links->prev){}
			for(i=0/*debug*/; links!=NULL; links=links->next){
//				fprintf(stderr, "link->num_msg = %d\n", link->num_msg);//debug
				for(link=links->link; link->num_msg!=0;){
//					fprintf(stderr, "will read msg from miner: %d\n", links->miner_id); //debug
					read_msg(link);	
					if(process_msg(&miner->new_comer, links, miner)==-1){
						links = miner->links;
						break;
					}
					if(link->num_msg==0 && link->fgetblock==true && miner->new_chain!=NULL){
						link->fgetblock=false;
						get_blocks(link, miner->blocks, miner->new_chain);
					}
					else
						link->fgetblock=false;
				} 
				i++; //debug
				if(links==NULL)
					break;
				if(links->next==NULL)
					break;
			}
		}
//		fprintf(stderr, "num of links: %d\n", i);//debug
		if(miner->links!=NULL){
			for(links=miner->links; links->prev!=NULL; links=links->prev){}
			for(; links!=NULL; links=links->next){
				link = links->link;
//				fprintf(stderr, "links dest: %p, new: %p, id: %d\n", link->dest, links->new_comer, links->miner_id);
			}
			if(i<miner->least && miner->links!=NULL && ((miner->links)->link)->dest!=(miner->links)->new_comer){
				n = rand()%(i);
				for(links=miner->links; links->prev!=NULL; links=links->prev){}
				for(i=0; i<n; i++){
					if(links->next==NULL)
						break;
					links=links->next;
				}
				if(links!=NULL)
					getaddr(links->link, miner->miner_id);
			}
		}
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id, miner);
	
	if(sim_time>=SIM_TIME-1){
		fprintf(stderr, "miner->blocks = %p ", miner->blocks);
		if(miner->blocks!=NULL){
			fprintf(stderr, "height = %d\n", ((miner->blocks)->block)->height);
		}else{fprintf(stderr, "height = 0\n");}
/*		if(miner->blocks!=NULL){
			for(blocks=miner->blocks; blocks->prev!=NULL; blocks=blocks->prev){}
			fprintf(stderr, "in main chain: ");
			for(; blocks!=NULL; blocks=blocks->next){
				fprintf(stderr, "%d, ", (blocks->block)->height);
			}
			fprintf(stderr,"\n");
		}
*//*		if(miner->new_chain!=NULL){
			for(blocks=miner->new_chain; blocks->prev!=NULL; blocks=blocks->prev){}
			fprintf(stderr, "in new_chain: ");
			for(; blocks!=NULL; blocks=blocks->next){
				fprintf(stderr, "%d, ", (blocks->block)->height);
			}
			fprintf(stderr, "\n");
		}
*/	}
}

#endif
