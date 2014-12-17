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
	bool			getblocks_sent=0;
	int 			i, n;
	struct links 	*links;
	struct link		*link;
	struct killed	*killed;
//	fprintf(stderr, "entered miner_routine()\n"); //debug
#ifdef DEBUG	
//	if(sim_time>=SIM_TIME-1){
		fprintf(stderr, "miner->blocks = %p ", miner->blocks);
		if(miner->blocks!=NULL){
			fprintf(stderr, "height = %d\n", ((miner->blocks)->block)->height);
		}else{fprintf(stderr, "height = 0\n");}
		if(miner->blocks!=NULL){
			struct blocks *blocks;
			for(blocks=miner->blocks; blocks->prev!=NULL; blocks=blocks->prev){}
			fprintf(stderr, "in main chain: ");
			for(; blocks!=NULL; blocks=blocks->next){
				fprintf(stderr, "%d, ", (blocks->block)->height);
			}
			fprintf(stderr,"\n");
		}

//#endif
//#ifdef DEBUG
		if(miner->new_chain!=NULL){
			struct blocks *blocks;
			for(blocks=miner->new_chain; blocks->prev!=NULL; blocks=blocks->prev){}
			fprintf(stderr, "in new_chain: ");
			for(; blocks!=NULL; blocks=blocks->next){
				fprintf(stderr, "%d, ", (blocks->block)->height);
			}
			fprintf(stderr, "\n");
		}
//	}
#endif

	if(miner->boot == true && miner->seed == true){
		for(i=0; i<NUM_DNS; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
		for(i=0; i<rand()%NUM_DNS; i++){}
		dns_query(&dns[i], &miner->new_comer, miner->miner_id);
//		fprintf(stderr, "sent dns_query\n");
		miner->boot = false;
//		miner->seed = false;
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
			getblocks_sent=false;
			for(;links->prev!=NULL; links=links->prev){}
			for(i=0/*debug*/; links!=NULL; links=links->next){
//				fprintf(stderr, "link->num_msg = %d\n", link->num_msg);//debug
				for(link=links->link; link!=NULL;){
//					fprintf(stderr, "will read msg from miner: %d\n", links->miner_id); //debug
					for(killed=dead; killed!=NULL; killed=killed->next){
						if(links->miner_id==killed->id){
							free_link(links, miner);
							links= miner->links;
							if(links!=NULL)
								link = links->link;
							else
								link = NULL;
						}
					}
					if(link->num_msg==0)
						break;
					read_msg(link);	
					if(process_msg(&miner->new_comer, links, miner)==-1){
						links=miner->links;
						if(links!=NULL)
							link=links->link;
						else
							link=NULL;
						break;
					}
					if(link!=NULL){
						if(link->num_msg==0 && link->fgetblock==true && miner->new_chain!=NULL && getblocks_sent==false){
							link->fgetblock=false;
							get_blocks(link, miner->blocks, miner->new_chain);
							getblocks_sent=true;
						}
					}
//					else
//						link->fgetblock=false;
					
				} 
				if(miner->new_chain!=NULL && getblocks_sent !=true && miner->links!=NULL){
					i = 0;
					for(links=miner->links; links->next!=NULL; links=links->next){
						i++;
					}
					if(i>0)
						n = rand()%i;
					else
						n = 0;
					for(i=0; i<n && links->prev!=NULL; links=links->prev){}
					get_blocks(links->link, miner->blocks, miner->new_chain);
					getblocks_sent = true;
				}
				i++; //debug
				if(links==NULL)
					break;
				if(links->next==NULL)
					break;
			}
		}
#ifdef DEBUG
		fprintf(stderr, "num of links: %d\n", i);//debug
#endif
		if(miner->links!=NULL){
			for(links=miner->links; links->prev!=NULL; links=links->prev){}
#ifdef DEBUG
			fprintf(stderr, "links:");
#endif
			for(; links!=NULL; links=links->next){
				link = links->link;
#ifdef DEBUG
//				fprintf(stderr, "links dest: %p, new: %p, id: %d\n", link->dest, links->new_comer, links->miner_id);
				fprintf(stderr, " id= %d", links->miner_id);
#endif
			}
#ifdef DEBUG
				fprintf(stderr, "\n");
#endif
			if(i<miner->least){
				if(i!=0)
					n = rand()%(i);
				else
					n=0;
				for(links=miner->links; links->prev!=NULL; links=links->prev){}
				for(i=0; i<n; i++){
					if(links->next==NULL)
						break;
					links=links->next;
				}
				if(links!=NULL){
					if(links->link == NULL){
						struct links *next, *prev;
						next=links->next;
						prev=links->prev;
						free(links);
						if(prev!=NULL)
							prev->next=next;
						if(next!=NULL)
							next->prev=prev;
						if(next!=NULL)
							miner->links=next;
						else if(prev!=NULL)
							miner->links=prev;
						else{
							miner->links=NULL;
							miner->boot=true;
							return;
						}
					}
					if(links->link->dest!=links->new_comer)
						getaddr(links->link, miner->miner_id);

				}
			}
		}
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id, miner);
}

#endif
