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
#define min(A,B) (((A)<(B))?(A):(B)) 

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
	struct links 	*links;//, *tmp_bad;//, *tmp_links;
	struct link		*link;//, *tmp_link;
	struct killed	*killed;
//#ifdef	DEBUG
/*	struct links *tmp_bad;
	int group;
	if(sim_time%600==0){
		fprintf(stderr, "miner: %d\n", miner->miner_id);
		if(miner->outbound!=NULL){
			fprintf(stderr, "outbound: ");
			for(links=miner->outbound; links->prev!=NULL; links=links->prev){}
			for(; links!=NULL; links=links->next){
				group = -1;
				if(bad_links!=NULL){
					for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
					for(;tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
						if(tmp_bad->miner_id==links->miner_id)
							group = tmp_bad->group;
					}
				}
				fprintf(stderr, "id= %d g= %d, ", links->miner_id, group);
			}
		}
		fprintf(stderr, "\ninbound: ");
		if(miner->inbound!=NULL){
			for(links=miner->inbound; links->prev!=NULL; links=links->prev){}
			for(; links!=NULL; links=links->next){
				group = -1;
				if(bad_links!=NULL){
					for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
					for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
						if(tmp_bad->miner_id==links->miner_id)
							group = tmp_bad->group;
					}
				}
				fprintf(stderr, "id= %d g= %d, ", links->miner_id, group);
			}	
		}
		fprintf(stderr, "\n");
	}
*/
//#endif	//DEBUG
//	fprintf(stderr, "entered miner_routine()\n"); //debug
#ifdef DEBUG	
	if(sim_time>=SIM_TIME-1/* && miner->seed == true*/){
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
	}
#endif //DEBUG
	if(miner->boot == true){
		memset(miner->addrman.v_random, 0, sizeof(miner->addrman.v_random));
		memset(miner->addrman.vv_new, 0, sizeof(miner->addrman.vv_new));
		memset(miner->addrman.vv_tried, 0, sizeof(miner->addrman.vv_tried));
	}
	if(miner->boot == true && miner->seed == true){
		for(i=0; i<NUM_DNS; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
		i=rand()%NUM_DNS;
		add_fixed_seeds(&miner->addrman);
		miner->boot = false;
	}
	else if(miner->boot == true){
		add_fixed_seeds(&miner->addrman);
		miner->boot = false;
	}
	else{
		int noutbound=0;
		if(miner->outbound!=NULL){
			for(links=miner->outbound; links->next!=NULL; links=links->next){}
			for(noutbound=0; links!=NULL; links=links->prev){
				noutbound++;
			}
		}
		struct caddrinfo *addr;
#ifdef ADDR_DEBUG
		if(miner->addrman.caddrinfo!=NULL){
			for(addr=miner->addrman.caddrinfo; addr->next!=NULL; addr=addr->next){}
			fprintf(stderr, "in caddfinfo: ");
			for(; addr!=NULL; addr=addr->prev){
				fprintf(stderr, "miner_id: %d, nid; %d new_comer: %p, f_in_tried = %d, ", addr->miner_id, addr->nid, addr->new_comer, addr->f_in_tried);
			}
		}
		fprintf(stderr, "\n");
#endif
//		miner->connect_count--;
//		if(miner->connect_count==0){
		addr = addrman_select(&miner->addrman, min(noutbound,8)+10);
#ifdef DEBUG
//		int k; 
//		for(k=0; k<SEED_NUM; k++)
//			fprintf(stderr, "seed[%d].new_comer = %p\n", k, &seeds[k].new_comer);
		if(addr!=NULL)
			fprintf(stderr, "*addr: id = %d, subnet = %d,  new_comer = %p\n", addr->miner_id, addr->subnet, addr->new_comer);
#endif
		miner->addrman.n_tries++;
		if(addr!=NULL){
			int subnet = addr->subnet;
			bool subnet_found=false;
			if(addr->miner_id==miner->miner_id||addr->new_comer==&miner->new_comer)
				subnet_found=true;
			if(miner->outbound!=NULL&&subnet_found!=true){
				for(links=miner->outbound; links->next!=NULL; links=links->next){}
				for(; links!=NULL; links=links->prev){
					if(subnet==links->subnet || addr->new_comer==links->new_comer||addr->miner_id==links->miner_id){
						subnet_found=true;
						break;
					}
				}
			}
			if(miner->inbound!=NULL&&subnet_found!=true){
				for(links=miner->inbound; links->next!=NULL; links=links->next){}
				for(; links!=NULL; links=links->prev){
					if(addr->new_comer==links->new_comer||addr->miner_id==links->miner_id){
						subnet_found=true;
						break;
					}
				}
			}
			if(dead!=NULL&&subnet_found!=true)
				for(killed=dead; killed!=NULL; killed=killed->next){
					if(killed->id == addr->miner_id)
						subnet_found=true;
				}

			if(subnet_found==true){}
			else if(miner->addrman.n_tries > 100){
				miner->addrman.n_tries = 0;
//				miner->boot = true;
			}
			else if(miner->n_outbound >= 8||(sim_time - addr->n_last_try < 600 && miner->addrman.n_tries < 30)){}
			else{
//				fprintf(stderr, "sent version\n");
				version(miner->miner_id, miner->subnet, addr->miner_id, addr->new_comer, &miner->new_comer, miner);
				miner->outbound->subnet=subnet;
//				miner->n_outbound++;
			}
		}
//		miner->connect_count=5;
//		}
		for(link=&miner->new_comer; link->num_msg!=0;){
//			fprintf(stderr, "new_comer link\n"); //debug
			read_msg(link);
			process_new(&miner->new_comer, miner);
//			fprintf(stderr, "miner->links = %p\n", miner->links);
		}
		memset(&miner->new_comer.buf[0], 0, BUF_SIZE);
		memset(&miner->new_comer.process_buf[0], 0, BUF_SIZE);
		links = miner->inbound;
		if(links==NULL){
			miner->n_inbound=0;
//			if(miner->addrman==NULL)
//			miner->boot=true;
		}
		else{
			getblocks_sent=false;
			if(miner->inbound!=NULL){
				for(links=miner->inbound;links->next!=NULL; links=links->next){}
				for(i=0/*debug*/; links!=NULL; /*links=links->prev*/){
					for(link=links->link;  link->num_msg!=0/*link!=NULL*/;){
#ifdef DEBUG	
						fprintf(stderr, "will read msg from miner: %d\n", links->miner_id); //debug
#endif
						if(link->num_msg==0)
							break;
						read_msg(link);	
						if(process_msg(&miner->new_comer, links, miner)==-1){
							links=miner->inbound;
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
					}	 
					if(links!=NULL)
						links=links->prev;
				}
			}
		}
		links = miner->outbound;
		if(links==NULL){
			miner->n_outbound=0;
//			if(miner->addrman==NULL)
//			miner->boot=true;
		}
		else{
#ifdef DEBUG
//			fprintf(stderr, "miner->outbound->link->num_msg == %d\n", miner->outbound->link->num_msg);
#endif
			getblocks_sent=false;
			if(miner->outbound!=NULL){
				for(links=miner->outbound;links->next!=NULL; links=links->next){}
				for(i=0/*debug*/; links!=NULL; /*links=links->prev*/){
					for(link=links->link; link->num_msg!=0/*link!=NULL*/;){
#ifdef DEBUG
						fprintf(stderr, "will read msg from miner: %d\n", links->miner_id); //debug
#endif
						if(link->num_msg==0)
							break;
						read_msg(link);	
						if(process_msg(&miner->new_comer, links, miner)==-1){
							links=miner->outbound;
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
					} 
					if(links!=NULL)
						links=links->prev;
				}
			}
		}

		if(miner->new_chain!=NULL && getblocks_sent !=true && miner->outbound!=NULL){
			i = 0;
	//		tmp_links=links;
			for(links=miner->outbound; links->next!=NULL; links=links->next){
				i++;
			}
			if(i>0)
				n = rand()%i;
			else
				n = 0;
			for(i=0; i<n && links->prev!=NULL; links=links->prev){}
			get_blocks(links->link, miner->blocks, miner->new_chain);
			getblocks_sent = true;
	//		links=tmp_links;
	//		link=links->link;
		}
#ifdef DEBUG
		fprintf(stderr, "num of links: %d\n", miner->n_outbound+miner->n_inbound);//debug
#endif
//		if(miner->n_addrman==0){
//			miner->boot = true;
//			return;
//		}
		if(miner->inbound!=NULL){
			for(links=miner->inbound; links->next!=NULL; links=links->next){}
#ifdef DEBUG
			fprintf(stderr, "inbound:");
			for(; links!=NULL; links=links->prev){
				fprintf(stderr, " id= %d", links->miner_id);
				i++;
			}
				fprintf(stderr, "\n");
#endif
			if(miner->n_outbound<MAX_OUTBOUND_CONNECTIONS){//will fix
			}
		}
	}
	miner->blocks = mine_block(miner->blocks, miner->miner_id, miner);
}

#endif
