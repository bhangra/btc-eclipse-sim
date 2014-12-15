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
#include"global.c"
#define GROUPS 2

//struct links	*bad_links;
//bool	is_bad_dns[NUM_DNS];	

void bad_addr(struct link *dest, struct miner *me, unsigned int dest_id){
	unsigned int			payload_size, set_size, sets;	
//	int 					i;
	struct link				*link;//, *tmp;
	struct links			*links, *tmp;

	bool					exists;
	unsigned int			dest_group;
	struct links			*tmp_bad;
	
	link			= dest;
	payload_size	= 0;
	set_size		= sizeof(struct link*) + sizeof(unsigned int);
	
	for(tmp_bad=bad_links; bad_links!=NULL; bad_links=bad_links->prev){
		if(tmp_bad->miner_id==dest_id){
			exists = 1;
			break;
		}
	}
	if(exists==true){
		dest_group	= tmp_bad->group;
	}
	else{
		dest_group = rand()%GROUPS;
		for(tmp=me->links; tmp->prev!=NULL; tmp=tmp->prev){}
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
//might change the maximum num of addrs to num of least neighbor
	for(tmp_bad=bad_links; tmp_bad!=NULL && 16+sets*set_size<BUF_SIZE; tmp_bad=tmp_bad->prev){
		if(tmp_bad->miner_id!=dest_id && tmp_bad->group == dest_group){
			memcpy(&link->sbuf[16+(sets*set_size)], &links->miner_id, sizeof(unsigned int));
			memcpy(&link->sbuf[16+(sets*set_size)+sizeof(unsigned int)], &links->new_comer, sizeof(struct link*));
			sets++;
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
		fprintf(stderr, "seed request received\n");
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
	unsigned int        i, miner_id, size, set, num_addr, payload_size;
	struct link         *link, *dest;
	struct links        *tmp;
	const struct msg_hdr *hdr;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
	fprintf(stderr, "command: %s\n", hdr->command); //debug
//ignore block related commands

//change addr() to bad_addr()
	if(strncmp(hdr->command, "getaddr", 7)==0){
		fprintf(stderr, "getaddr received\n");
		memcpy(&miner_id, &link->process_buf[16], sizeof(unsigned int));
		bad_addr(link, me, miner_id);
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
			fprintf(stderr, "will not send version to me\n");
			return NULL;
		}
		
		/*return*/tmp = version(me->miner_id, miner_id, dest, &me->new_comer, me->links);
		if(tmp!=NULL){
			if(bad_links==NULL){
				bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
				bad_links->group = rand()%GROUPS;
			}
			else{
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
		}
		return tmp;
	}
	else if(strncmp(hdr->command, "version", 7)==0){
		fprintf(stderr, "version received\n");
//		return verack(new_comer, me->links);
		tmp = verack(new_comer, me->links);
		
		if(tmp!=NULL){
			if(bad_links==NULL){
				bad_links = add_links(tmp->miner_id, tmp->new_comer, tmp->new_comer, bad_links);
				bad_links->group = rand()%GROUPS;
			}
			else{
				for(tmp_bad=bad_links; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
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
	return me->links;
}

void bad_miner_routine(struct miner *miner){
	int				i;
	struct links	*links;
	struct link		*link;
/*	if(miner->boot == true && miner->seed == true){
		for(i=0; i<5; i++){
			dns_seed(miner->miner_id, &dns[i], &miner->new_comer);
		}
		miner->seed = false;
	}
*/	if(miner->boot == true){
		for(i=0; i<NUM_DNS; i++){
			dns_query(&dns[i/*rand()%5*/], &miner->new_comer, miner->miner_id);
		}
		miner->boot = false;
	}
	else{
		for(link=&miner->new_comer; link->num_msg!=0;){
			fprintf(stderr, "new_comer link\n"); //debug
			read_msg(link);
			miner->links = process_bad_new(&miner->new_comer, miner);
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
