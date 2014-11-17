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

struct links *process_bad_dns(struct link *new_comer, struct links *seeds){
	char                    *payload;
	unsigned int            dest_id;
	struct link             *link;
	struct links            *tmp;
	const struct msg_hdr    *hdr;
	link    = new_comer;
	hdr     = (struct msg_hdr*)(link->process_buf);
	payload = (char *)(hdr+sizeof(struct msg_hdr));
	if(strncmp(hdr->command, "dnsseed", 7)==0){
		fprintf(stderr, "seed request received\n");
		tmp = seed_receive(link, seeds);
		if(tmp!=NULL)
			return tmp;
		else
   			return seeds;
	}
	else if(strncmp(hdr->command, "dnsquery", 8)==0){
		fprintf(stderr, "dnsquery received\n"); //debug
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
	bool                connect, connected;
	char                *payload;
	unsigned int        height, height2, i, miner_id, size, set, num_addr, payload_size;
	struct link         *link, *dest;
	struct links        *tmp;
	const struct msg_hdr *hdr;
	struct block        *block;
	struct blocks       *blocks;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
	payload = (char *)(hdr +sizeof(struct msg_hdr));
	fprintf(stderr, "command: %s\n", hdr->command); //debug
	if(strncmp(hdr->command, "getaddr", 7)==0){
		fprintf(stderr, "getaddr received\n");
		memcpy(&miner_id, &link->process_buf[16], sizeof(unsigned int));
		addr(link, me, miner_id);
	}
	else if(strncmp(hdr->command, "addr", 4)==0){
		memcpy(&payload_size, &link->process_buf[12], sizeof(unsigned int));
//      num_addr	= (hdr->message_size/(size+sizeof(unsigned int)));
		size		= sizeof(struct link*);
		set			= size + sizeof(unsigned int);
		num_addr	= payload_size/set;
		fprintf(stderr, "num_addr = %d\n", num_addr);
		if(num_addr == 0)
			me->boot = true;
		connected = false;
		for(i=0; i<num_addr; i++){
			memcpy(&miner_id, &link->process_buf[16+i*set], sizeof(unsigned int));
			memcpy(&dest, &link->process_buf[16+i*set+sizeof(unsigned int)], size);
			connect		= true;
			if(miner_id==me->miner_id){
				connect	= false;
			}
			else{
				for(tmp=me->links; tmp->prev!=NULL; tmp=tmp->prev){}
				for(; tmp->next!=NULL; tmp=tmp->next){
					if(tmp->miner_id==miner_id){
						connect = false;
						break;
					}
				}
			}
			if(connect){
				connected = true;
				fprintf(stderr, "will send version to dest: %p, id: %d\n", dest, miner_id);
				me->links = version(me->miner_id, miner_id, dest, &me->new_comer, me->links);
			}
		}
		if(!connected){
			dns_query(&dns[rand()%5], &me->new_comer, me->miner_id);
		}
	}
	else if(strncmp(hdr->command, "verack", 6)==0){
		memcpy(&link->dest, &link->process_buf[16], sizeof(struct link*));
		fprintf(stderr, "received verack with link: %p\n", link->dest);
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
					process_bad_msg(&miner->new_comer, links, miner);
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
