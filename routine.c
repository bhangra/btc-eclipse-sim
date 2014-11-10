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

void dns_routine(struct dns *dns){
	fprintf(stderr, "\n");
	for(; (&dns->new_comer)->num_msg!=0;){
		fprintf(stderr, "num of msgs to dns: %d\n", (&dns->new_comer)->num_msg);
		read_msg(&dns->new_comer);
		fprintf(stderr, "read msg to dns\n");
		dns->seeds = process_dns(&dns->new_comer, /*(struct links*)&*/dns->seeds);
	}
}

void miner_routine(struct miner *miner){
	int 			i;
	struct links 	*links;
	struct link		*link;
	fprintf(stderr, "entered miner_routine()\n"); //debug
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
		for(i=0; i<rand()%6; i++){}
		dns_query(&dns[i], &miner->new_comer, miner->miner_id);
		fprintf(stderr, "sent dns_query\n");
		miner->boot = false;
	}
	else{
		for(link=&miner->new_comer; link->num_msg!=0;){
			fprintf(stderr, "new_comer link\n"); //debug
			read_msg(link);
			miner->links = process_new(&miner->new_comer, links, miner);
			fprintf(stderr, "miner->links = %p\n", miner->links);
		}
		links = miner->links;
		if(miner->links==NULL){
			i=0;
			miner->boot=true;
			return;
		}
//			return;
		else{
			for(i=0/*debug*/; links!=NULL; links=links->next){
//				fprintf(stderr, "link->num_msg = %d\n", link->num_msg);//debug
				for(link=links->link; link->num_msg!=0;){
//					fprintf(stderr, "will read msgs\n"); //debug
					read_msg(link);	
					process_msg(&miner->new_comer, links, miner);
				} 
				i++; //debug
				if(links->next==NULL)
					break;
			}
		}
		fprintf(stderr, "num of links: %d\n", i);//debug
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id, miner);
	fprintf(stderr, "miner->blocks = %p ", miner->blocks);
	if(miner->blocks!=NULL){
		fprintf(stderr, "height = %d\n", ((miner->blocks)->block)->height);
	}else{fprintf(stderr, "height = 0\n");}
}
void links_kill(int miner_id, struct threads *threads){
	struct threads	*tmp;
	struct miner	*miner;
	struct links	*links, *after, *before;
	for(tmp = threads; tmp->prev==NULL;tmp=tmp->prev){}
	for(miner=tmp->miner; tmp!=NULL; tmp=tmp->next){
		for(links=miner->links; links!=NULL; links=links->next){
			if(links->miner_id==miner_id){
				remove_links(links);
			}
		}
	}
}
void miner_kill(struct miner *miner){
	
}

#endif
