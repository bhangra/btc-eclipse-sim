#ifndef BAD_ROUTINE_C
#define BAD_ROUTINE_C
#ifdef BAD_NODES
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"action.c"
#include"global.c"
//#define GROUPS 2

//struct links	*bad_links;
//bool	is_bad_dns[NUM_DNS];	

void hexDump (char *desc, void *addr, int len);
#ifdef BAD_NODES
unsigned int bad_count[BAD_NODES];
#endif
void bad_addr(struct link *dest, struct miner *me, unsigned int dest_id){
	unsigned int			payload_size, set_size, sets;	
//	int 					i;
	struct link				*link;//, *link_tmp;//, *tmp;
	struct links			/**links,*/ *tmp;
//	struct miner			*miner_tmp;
//	struct threads			*threads;
	struct bad_threads		*bad;
	bool					exists;
	unsigned int			dest_group;
	struct links			*tmp_bad = NULL;
	
	link			= dest;
	payload_size	= 0;
	set_size		= sizeof(struct link*) + sizeof(unsigned int)*3;
	exists=false;
	for(tmp_bad=bad_links; bad_links!=NULL; bad_links=bad_links->prev){
		if(tmp_bad->miner_id==dest_id){
			exists = true;
			break;
		}
	}
	if(exists==true){
		dest_group	= tmp_bad->group;
	}
	else{
		dest_group = rand()%GROUPS;
		for(tmp=me->outbound; tmp->prev!=NULL; tmp=tmp->prev){}
		for(; tmp!=NULL && tmp->miner_id!=dest_id; tmp=tmp->next){}
		if(tmp!=NULL){
			bad_links=add_links(dest_id, tmp->new_comer, tmp->new_comer, bad_links);
			dest_group			= rand()%GROUPS;
			bad_links->group	= dest_group;
		}
		else{
			dest_group = rand()%GROUPS;
		}
	}
	memcpy(&link->sbuf[0], "addr", 4);
	sets = 0;
//	if(bad_threads!=NULL)
//		for(bad=bad_threads; bad->prev!=NULL; bad=bad->prev){}
	struct link *link_tmp;
	struct miner *miner_tmp;
	for(bad=bad_threads; sets<BAD_NODES; bad=bad->next){
		miner_tmp	= bad->thread->miner;
		link_tmp	= &miner_tmp->new_comer;
		memcpy(&link->sbuf[16+(sets*set_size)], &(bad->thread->miner)->miner_id, sizeof(unsigned int));
		memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)], &link_tmp, sizeof(struct link*));
		memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)+sizeof(struct link*)], &sim_time, sizeof(unsigned int));
		memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)+sizeof(struct link*)+sizeof(unsigned int)], &bad->miner->subnet, sizeof(unsigned int));
		sets++;
	}

	if(bad_links!=NULL){
		for(tmp_bad=bad_links; tmp_bad!=NULL && 16+(sets*set_size)-sets<BUF_SIZE; tmp_bad=tmp_bad->prev){
			if(tmp_bad->miner_id!=dest_id && tmp_bad->group == dest_group){
				memcpy(&link->sbuf[16+(sets*set_size)], &tmp_bad->miner_id, sizeof(unsigned int));
				memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)], &tmp_bad->new_comer, sizeof(struct link*));
				memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)+sizeof(struct link*)], &sim_time, sizeof(unsigned int));
				memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)+sizeof(struct link*)+sizeof(unsigned int)], &tmp_bad->subnet, sizeof(unsigned int));

				sets++;
			}
		}
	}
	payload_size = set_size*sets;
	memcpy(&link->sbuf[12], &payload_size, sizeof(unsigned int));
	send_msg(link->dest, (char *)link->sbuf, 16+payload_size);
}

struct links *process_bad_dns(struct link *new_comer, struct links *seeds){
	bool					exists;
	struct link             *link;
	struct links            *tmp, *tmp_bad;
	const struct msg_hdr    *hdr;
	link    = new_comer;
	hdr     = (struct msg_hdr*)(link->process_buf);
	if(strncmp(hdr->command, "dnsseed", 7)==0){
#ifdef DEBUG
		fprintf(stderr, "seed request received\n");
#endif
		tmp = seed_receive(link, seeds);
		if(tmp!=NULL){
			exists = 0;
			if(bad_links!=NULL){
				for(tmp_bad=bad_links; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==tmp->miner_id){
						exists = true;
						break;
					}
				}
				if(exists==false){
					bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
					bad_links->group = rand()%GROUPS;
				}
			}
			else{
				bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
			}
			return tmp;
		}
		else
   			return seeds;
	}
	else if(strncmp(hdr->command, "dnsquery", 8)==0){
#ifdef DEBUG
		fprintf(stderr, "dnsquery received\n"); //debug
#endif
		dns_roundrobin(link, seeds);
		return seeds;
	}
	else{
		return seeds;
	}
}

void bad_dns_routine(struct dns *dns){
	fprintf(stderr, "\n");
	for(; (&dns->new_comer)->num_msg!=0;){
		fprintf(stderr, "num of msgs to dns: %d\n", (&dns->new_comer)->num_msg);
		read_msg(&dns->new_comer);
		fprintf(stderr, "read msg to dns\n");
		dns->seeds = process_bad_dns(&dns->new_comer, /*(struct links*)&*/dns->seeds);
	}
}

int process_bad_msg(struct link *new_comer,struct links *links, struct miner *me){
	bool                connect, /*connected,*/ exists;
	unsigned int        i, miner_id, size, set, num_addr, payload_size, dgroup, subnet;//, n_time;
	struct link         *link, *dest;
	struct links        *tmp, *tmp_bad;
	const struct msg_hdr *hdr;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
#ifdef DEBUG
	fprintf(stderr, "command: %s\n", hdr->command); //debug
#endif
//ignore block related commands
	if(strncmp(hdr->command, "getaddr", 7)==0){
#ifdef DEBUG
		fprintf(stderr, "getaddr received\n");
#endif
		memcpy(&miner_id, &link->process_buf[16], sizeof(unsigned int));
		bad_addr(link, me, miner_id);
	}
	else if(strncmp(hdr->command, "addr", 4)==0){
		memcpy(&payload_size, &link->process_buf[12], sizeof(unsigned int));
      num_addr	= (hdr->message_size/(size+sizeof(unsigned int)));
		size		= sizeof(struct link*);
		set			= size + sizeof(unsigned int)*3;
		num_addr	= payload_size/set;
//#ifdef DEBUG
		fprintf(stderr, "num_addr = %d\n", num_addr);
//#endif
//		if(num_addr == 0)
//			me->boot = true;
//		connected = false;
//int process_bad_msg(struct link *new_comer,struct links *links, struct miner *me)
		for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
		for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
			if(tmp_bad->miner_id==links->miner_id){
				dgroup = tmp_bad->group;
				break;
			}
		}
		if(tmp_bad==NULL)
			dgroup=rand()%2;
		for(i=0; i<num_addr; i++){
			memcpy(&miner_id, &link->process_buf[16+i*set], sizeof(unsigned int));
			memcpy(&dest, &link->process_buf[16+i*set+sizeof(unsigned int)], size);
//			memcpy(&n_time, &link->process_buf[16+i*set+sizeof(unsigned int+size)], sizeof(unsigned int));
			memcpy(&subnet, &link->process_buf[16+i*set+sizeof(unsigned int)*2+size], sizeof(unsigned int));
			connect		= true;
			if(miner_id==me->miner_id){
				connect	= false;
			}
			else{
				if(me->outbound!=NULL){
					for(tmp=me->outbound; tmp->prev!=NULL; tmp=tmp->prev){}
					for(; tmp->next!=NULL; tmp=tmp->next){
						if(tmp->miner_id==miner_id){
							connect = false;
							break;
						}
					}
				}
				if(me->inbound!=NULL){
					for(tmp=me->inbound; tmp->prev!=NULL; tmp=tmp->prev){}
					for(; tmp->next!=NULL; tmp=tmp->next){
						if(tmp->miner_id==miner_id){
							connect = false;
							break;
						}
					}
				}
			}
			struct killed *killed;
			if(dead!=NULL)
				for(killed=dead; killed!=NULL; killed=killed->next){
					if(killed->id == miner_id)
						connect=false;
				}
			if(connect){
//				connected = true;
#ifdef DEBUG
				fprintf(stderr, "will send version to dest: %p, id: %d\n", dest, miner_id);
#endif
				version(me->miner_id, me->subnet, miner_id, dest, &me->new_comer, me);
				exists = false;
				tmp = me->outbound;
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==tmp->miner_id){
						exists = true;
						break;
					}
				}
				if(exists==false){
					bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
					bad_links->group = dgroup;
					bad_links->subnet= subnet;
				}
			}
		}
/*		if(!connected){
			dns_query(&dns[rand()%5], &me->new_comer, me->miner_id);
		}
*/	}
	else if(strncmp(hdr->command, "verack", 6)==0){
		memcpy(&link->dest, &link->process_buf[16], sizeof(struct link*));
#ifdef DEBUG
		fprintf(stderr, "received verack with link: %p\n", link->dest);
#endif
    }
	return 1;
}

struct links *process_bad_new(struct link *new_comer, struct miner *me){
	unsigned int        miner_id, size, payload_size;
	struct link         *link, *dest;
	struct msg_hdr *hdr;

	bool				exists;
	struct links 		*tmp, *tmp_bad;


	tmp = NULL;
	size = sizeof(struct link*);
	link = new_comer;
	hdr = (struct msg_hdr*)(link->process_buf);
	memcpy(&payload_size, &link->process_buf[12], 4);
//	fprintf(stderr, "check command\n"); //debug
//	hexDump("Message received", link->process_buf, 16+hdr->message_size);
	if(strncmp(hdr->command, "roundrobin", 10)==0){
//		memcpy(&link, &link->process_buf[16], size);
		memcpy(&dest, &link->process_buf[16], size);
		memcpy(&miner_id, &link->process_buf[16+size], sizeof(unsigned int));
		if(miner_id==me->miner_id){
#ifdef DEBUG
			fprintf(stderr, "will not send version to me\n");
#endif
			return NULL;
		}
		
		version(me->miner_id, me->subnet, miner_id, dest, &me->new_comer, me);
		if(tmp!=NULL){
			if(bad_links==NULL){
				bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
				bad_links->group = rand()%GROUPS;
			}
			else{
				exists = false;
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==tmp->miner_id){
						exists = true;
						break;
					}
				}
				if(exists==false){
					bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
					bad_links->group = rand()%GROUPS;
				}
			}
		}
		if(tmp!=NULL)
			return tmp;
		else
			return me->outbound;
	}
	else if(strncmp(hdr->command, "version", 7)==0){
#ifdef DEBUG
		fprintf(stderr, "version received\n");
#endif
//		return verack(new_comer, me->links);
		tmp = verack(new_comer, me->inbound, me);
		
		if(tmp!=NULL){
			if(bad_links==NULL){
				bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
				bad_links->group = rand()%GROUPS;
			}
			else{
				exists = false;
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next);
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==tmp->miner_id){
						exists = true;
						break;
					}
				}
				if(exists==false){
					bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
					bad_links->group = rand()&GROUPS;
				}
			}
		}
		return tmp;
	}
	return me->inbound;
}

void bad_miner_routine(struct miner *miner){
	int				i;
	struct links	*links;
	struct link		*link;
//	struct links	*tmp;
	struct links	*tmp_bad=NULL;
	bool			exists;
//	unsigned int	dest_group;
/*	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
		miner->seed = false;
	}
*/	if(miner->boot == true){

		bad_count[miner->miner_id-SEED_NUM]=0;
		for(i=0; i<SEED_NUM; i++){
			version(miner->miner_id, miner->subnet, seeds[i]. miner_id, &seeds[i].new_comer, &miner->new_comer, miner);
			exists = false;
			if(bad_links!=NULL){
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
					for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
						if(tmp_bad->miner_id==seeds[i].miner_id){
							exists = true;
							break;
					}
				}
			}
			if(exists==false){
				bad_links=add_links(seeds[i].miner_id, &seeds[i].new_comer, &seeds[i].new_comer, bad_links);
				bad_links->group = rand()%GROUPS;
//				miner->outbound=add_links(seeds[i].miner_id, &seeds[i].new_comer, &seeds[i].new_comer, miner->outbound);
//				miner->outbound->group = bad_links->group;
			
			}
//			dns_query(&dns[i/*rand()%5*/], &miner->new_comer, miner->miner_id);
		}
		miner->boot = false;
	}

	else{
		bad_count[miner->miner_id-SEED_NUM]++;
		if(bad_count[miner->miner_id-SEED_NUM]>=30){
			for(i=0; i<SEED_NUM; i++){
				for(tmp_bad=miner->outbound; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next);
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==seeds[i].miner_id){
						bad_addr(tmp_bad->link/*&seeds[i].new_comer*/, miner, seeds[i].miner_id);
						getaddr(tmp_bad->link, miner->miner_id);
						break;
					}
				}
			}
			bad_count[miner->miner_id-SEED_NUM]=0;
		}
		
		for(link=&miner->new_comer; link->num_msg!=0;){
#ifdef DEBUG
			fprintf(stderr, "new_comer link\n"); //debug
#endif
			read_msg(link);
			miner->inbound = process_bad_new(&miner->new_comer, miner);
		}
		links = miner->outbound;
		if(links==NULL){
			i=0;
//			miner->boot=true;
		}
		else{
			for(;links->prev!=NULL; links=links->prev){}
			for(i=0/*debug*/; links!=NULL; links=links->next){
				for(link=links->link; link->num_msg!=0;){
					read_msg(link);
					process_bad_msg(&miner->new_comer, links, miner);
				}
				i++;
				if(links->next==NULL)
					break;
			}
		}
		links = miner->inbound;
		if(links==NULL){
			i=0;
//			miner->boot=true;
		}
		else{
			for(;links->prev!=NULL; links=links->prev){}
			for(i=0/*debug*/; links!=NULL; links=links->next){
				for(link=links->link; link->num_msg!=0;){
					read_msg(link);
					process_bad_msg(&miner->new_comer, links, miner);
				}
				i++;
				if(links->next==NULL)
					break;
			}
		}
	}
//	if(miner->links!=NULL){
//		if(i<LEAST_NEIGHBOR && miner->links!=NULL && ((miner->links)->link)->dest!=(miner->links)->new_comer){
//			getaddr((miner->links)->link, miner->miner_id);
//		}
//	}
}
#endif	//BAD_NODES
#endif	//BAD_ROUTINE_C
