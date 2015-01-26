#ifndef ACTION_C
#define ACTION_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<time.h>

#include"addrman.c"
#include"global.c"
#include"block.c"
#include"thread.c"
#include"connection.c"
#include"proto-node.h"


void add_block_record(struct miner *me, struct block *new, struct blocks *mine);

//totally for debugging
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        fprintf (stderr, "%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                fprintf (stderr, "  %s\n", buff);

            // Output the offset.
            fprintf (stderr,"  %04x ", i);
        }

        // Now the hex code for the specific character.
        fprintf (stderr, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        fprintf (stderr,"   ");
        i++;
    }

    // And print the final ASCII bit.
    fprintf (stderr,"  %s\n", buff);
}

void dns_seed(unsigned int my_id, struct dns *dns, struct link *new_comer, unsigned int subnet){
	struct link		*link;
	unsigned int	size, payload_size;
	size			= sizeof(struct link*);
	link			= new_comer; 
	link->dest		= &dns->new_comer;
	payload_size	= size + sizeof(unsigned int)*2;
	memcpy(&link->sbuf[0], "dnsseed", 7);
	memcpy(&link->sbuf[12], &payload_size, sizeof(unsigned int));
	memcpy(&link->sbuf[16], &link, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
	memcpy(&link->sbuf[16+size+sizeof(unsigned int)], &subnet, sizeof(unsigned int));
	send_msg(&dns->new_comer, (char *)link->sbuf, 16+payload_size);
}
struct links *seed_receive(struct link *new_comer, struct links *seeds){
	unsigned int	miner_id, size;
	struct link		*link; 
	struct links	*tmp;
	size = sizeof(struct link*);
	memcpy(&link, &new_comer->process_buf[16], size);
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	if(seeds!=NULL){
		for(tmp=seeds; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if(tmp->miner_id == miner_id)
				return seeds;
		}
	}
//	fprintf(stderr, "seed link: %p, id: %d\n", link, miner_id);
//	fprintf(stderr, "seed_receive(): will add_links()\n");
	return add_links(miner_id, link, link, seeds);
}
void dns_query(struct dns *dns, struct link *new_comer, unsigned int my_id){
	unsigned int	size, payload_size, miner_id;
	struct link		*link;
	size		= sizeof(struct link*);
	payload_size= size+sizeof(unsigned int);
	miner_id	= my_id;
	link		= new_comer;
	link->dest	= &(dns->new_comer);
	memcpy(&link->sbuf[0], "dnsquery", 8);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &new_comer, size);
	memcpy(&link->sbuf[16+size], &miner_id, sizeof(unsigned int));
	
	send_msg(&dns->new_comer, (char *)link->sbuf, 16+payload_size);  
}

void dns_roundrobin(struct link *new_comer,/* struct links *seeds,*/ unsigned int dns_num){
	unsigned int	i,/* j,*/ /*num_seeds,*/ size, payload_size, dest_id;/*, random;*/
	struct link		*link;
	struct miner	*tmp;//, *head;
	size			= sizeof(struct link*);
	payload_size	= size + sizeof(unsigned int)*2;
	link 			= new_comer;

	memcpy(&dest_id, &link->process_buf[16+size], sizeof(unsigned int));
//	if(seeds==NULL)
//		return;
//	for(tmp=seeds;tmp->prev!=NULL;tmp=tmp->prev){}
//	head=tmp;
/*	for(i=1; tmp->next!=NULL; i++){
		tmp=tmp->next;
	}
*/
//	num_seeds=i;
/*
	for(; ;){
		i=num_seeds;
		random=rand();
//		fprintf(stderr, "i = %d, random = %d\n", i, random);
		if(num_seeds<=0){
//			fprintf(stderr, "no seeds\n");
			return;
		}
		if(i==1){
			tmp=head;
			if(tmp->miner_id!=dest_id)
				break;
			else
				return;
		}
		if(random>=i){
			j=random%num_seeds;
//			if(j!=dest_id||(i==1&&dest_id==0)){
//				break;
//			}
//		}
			tmp = head;
			for(i=0; i<j; tmp=tmp->next){
				i++;//	tmp = tmp->next;
			}
			if(tmp->miner_id != dest_id){
				break;
			}
		}
	}
*/
	i=rand()%SEEDS_PER_DNS;
	tmp=seeds[dns_num+i];	
	struct link *dest=&tmp->new_comer;
//	fprintf(stderr, "seed: %d %p\n", tmp->miner_id, &tmp->new_comer);
	memcpy(&link->dest, &link->process_buf[16], size);
	memcpy(&link->sbuf[0], "roundrobin", 10);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &dest, size);
	memcpy(&link->sbuf[16+size], &tmp->miner_id, sizeof(unsigned int));
	memcpy(&link->sbuf[16+size+sizeof(unsigned int)], &tmp->subnet, sizeof(unsigned int));
	send_msg(link->dest, (char *)link->sbuf, 16+payload_size);
//	fprintf(stderr, "sent DNS round robin\n");
}

void getaddr(struct link *dest, unsigned int my_id){
	unsigned int	size, id;
	struct link 	*link;
	id	 = my_id;
	size = sizeof(unsigned int);
	link = dest;
	memset(link->sbuf, 0, sizeof(link->sbuf));
	memcpy(&link->sbuf[0], "getaddr", 7);
	memcpy(&link->sbuf[12], &size, size);
	memcpy(&link->sbuf[16], &id, size); 
	send_msg(link->dest, (char *)link->sbuf, 16+size);
}

void addr(struct link *dest, struct miner *me, unsigned int dest_id){
	unsigned int	size, set;
	unsigned int	i;
	struct link		*link;
//	struct links 	*links;
	link	= dest;
	size	= 0;
	set		= sizeof(struct link*) + sizeof(unsigned int) + sizeof(unsigned int) + sizeof(unsigned int);
	memcpy(&link->sbuf[0], "addr", 4);
	i = getaddr_(&me->addrman, &link->sbuf[16]);
/*	if(me->addrman==NULL)
		return;
	for(links=me->addrman; links->next!=NULL; links=links->next){}
	for(i=0; links!=NULL && (16+i*set) < (BUF_SIZE-set); i++){
		if(links->miner_id!=dest_id){
//			link=links->link;
//			fprintf(stderr, "adding to addr dest: %p, id: %d\n", links->new_comer, links->miner_id);
			memcpy(&link->sbuf[16+(i*set)], &links->miner_id, sizeof(unsigned int));	
			memcpy(&link->sbuf[16+(i*set)+sizeof(unsigned int)], &links->new_comer, sizeof(struct link*));
		}
		else{
			i=i-1;
		}
		links=links->prev;
	}
*/	size=i*set;
	memcpy(&link->sbuf[12], &size, 4);
	send_msg(link->dest, (char *)link->sbuf, 16+(set*i));
}
//dest is desination's new_comer
void version(unsigned int my_id, unsigned int my_subnet, unsigned int dest_id,  struct link *dest, struct link *new_comer, struct miner *miner, unsigned int subnet){
	struct links *new;//, *save;
	struct link *link, *tmp;
	unsigned int size, payload_size;
//	fprintf(stderr, "will send version msg\n"); //debug
#ifdef VER_DEBUG
	fprintf(stderr, "version to link: %p id: %d\n", dest, dest_id); //debug
#endif
	size = sizeof(struct link*);
	payload_size = size+sizeof(unsigned int)+size+sizeof(unsigned int);
//	save = links;
	new = add_links(dest_id, dest, dest, miner->outbound);
	new->subnet = subnet;
#ifdef VER_DEBUG
	fprintf(stderr, "version: new = %p , new->link = %p\n", new, new->link);
#endif
//	if(new==save)
//		return;
	link = new->link;
	tmp = new_comer;
#ifdef MEM_DEBUG
	fprintf(stderr, "sending new->link: %p\n", new->link);
#endif
	memcpy(&link->sbuf[0], "version", 7);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &link, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
	memcpy(&link->sbuf[16+size+sizeof(unsigned int)], &tmp, size);
	memcpy(&link->sbuf[16+size*2+sizeof(unsigned int)], &my_subnet, sizeof(unsigned int));
	send_msg(dest, (char *)link->sbuf, 16+payload_size);
#ifdef VER_DEBUG
	fprintf(stderr, "sent version to dest: %p, id: %d with mylink: %p\n", link->dest, new->miner_id, link); //debug
#endif
	miner->outbound = new;
	miner->n_outbound++;
#ifdef DEBUG
	fprintf(stderr, "miner->outbound = %p\n", miner->outbound);
	fprintf(stderr, "miner->outbound->link->num_msg == %d\n", miner->outbound->link->num_msg);
#endif
}

struct links *nat(struct link *new_comer, unsigned int miner_id, struct links *links, struct miner *me){
	unsigned int dest_id;
	struct links *tmp, *tmp2, *tmp3;//, *save;
	struct link *link, *dest, *dest_new_comer;
	unsigned int size;
	size = sizeof(struct link*);
	memcpy(&dest, &new_comer->process_buf[16], size);
//	memcpy(&dest_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	memcpy(&dest_new_comer, &new_comer->process_buf[16+size+sizeof(unsigned int)], sizeof(struct link*));
	dest_id = miner_id;
	struct killed *killed;
	if(dead!=NULL){
			for(killed=dead; killed!=NULL; killed=killed->next){
				if(killed->id == dest_id)
					return links;
			}
	}
//	fprintf(stderr, "nat: ");
#ifdef	VER_DEBUG
	fprintf(stderr, "will send nat to %d\n", dest_id);
#endif	//DEBUG
//	save		= links;
	tmp         = add_links(dest_id, dest, dest_new_comer, links);
/*	if(tmp==save){
		link = &me->new_comer;
		size = 0;
		link->dest = dest;
		memset(&link->sbuf, 0, sizeof(link->sbuf));
		memcpy(&link->sbuf[0], "nat", 3);
		memcpy(&link->sbuf[12], &size, 4);
		send_msg(link->dest, (char *)link->sbuf, 16);
		memset(&link->sbuf, 0, sizeof(link->sbuf));
	
		return me->inbound;
	}
*/		
	link        = tmp->link;

	size = 0;
	memset(&link->sbuf, 0, 16);
	memcpy(&link->sbuf[0], "nat", 3);
	memcpy(&link->sbuf[12], &size, 4);
	send_msg(link->dest, (char *)link->sbuf, 16);
#ifdef VER_DEBUG
	fprintf(stderr, "sent nat to link->dest %p\n", &link->dest);
#endif
	memset(&link->sbuf, 0, sizeof(link->sbuf));
	tmp2=tmp->prev;
	tmp3=tmp->next;
	free_link(tmp, me);
	if(tmp2!=NULL)
		tmp2->next=tmp3;
	if(tmp3!=NULL)
		tmp3->prev=tmp2;
//	if(me->inbound==tmp){
	if(tmp2!=NULL)
		me->inbound=tmp2;
	else
		me->inbound=tmp3;
//	}
	me->n_inbound++;
//	fprintf(stderr, "me->inbound = %p, tmp2 = %p, tmp = %p, tmp3 = %p\n", me->inbound, tmp2, tmp, tmp3);
	return me->inbound;
}

struct links *verack(struct link *new_comer, struct links *links, struct miner *me){
	unsigned int miner_id, subnet;
	struct links *tmp;//, *save;
	struct link *link, *dest, *dest_new_comer;
	unsigned int size;
	struct killed *killed;
//	fprintf(stderr, "will send verack msg\n"); //debug
	size = sizeof(struct link*);
//	dest		= (struct link*)&new_comer->process_buf[16];


	memcpy(&dest, &new_comer->process_buf[16], size);
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));

	if(dead!=NULL)
            for(killed=dead; killed!=NULL; killed=killed->next){
                if(killed->id == miner_id)
                    return links;
            }
	memcpy(&dest_new_comer, &new_comer->process_buf[16+size+sizeof(unsigned int)], sizeof(struct link*));
	memcpy(&subnet, &new_comer->process_buf[16+size*2+sizeof(unsigned int)], sizeof(unsigned int));
//	save		= links;
	tmp			= add_links(miner_id, dest, dest_new_comer, me->inbound);
	tmp->subnet	= subnet;
/*	if(tmp==save){
		memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
		return nat(new_comer, miner_id, links, me);
	}
*/	me->n_inbound++;
	link		= tmp->link;
#ifdef VER_DEBUG
	fprintf(stderr, "verack to dest: %p, id: %d, dest->write_pos: %p with mylink: %p &buf[0]: %p\n", dest, miner_id, &dest->write_pos, link, &link->buf[0]);
#endif
	memcpy(&link->sbuf[0], "verack", 6);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &link, size);
	send_msg(link->dest, (char *)link->sbuf, 16+size);
//	fprintf(stderr, "sent verack to: %d\n", tmp->miner_id); //debug
	memset(&link->sbuf, 0, sizeof(link->sbuf));
	return tmp;
}

void send_block(struct block *block, struct link *dest){
	int size;
	struct link *link;
	size = sizeof(struct block);
	link = dest;
	memcpy(&link->sbuf[0], "block", 5);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], block, size);
	send_msg(link->dest, (char *)link->sbuf, 16+size);
}

void propagate_block(struct block *block, struct miner *me){
	struct link		*link;
	struct links	*links;
//	fprintf(stderr, "me->links = %p\n", me->links);
	if(me->outbound==NULL&&me->inbound==NULL)
		return;
/*	//redundunt free_link
	struct killed *killed;
	for(links=me->links; links->next!=NULL;links=links->next){}
	for(; links!=NULL; links=links->prev){
		for(killed=dead; killed!=NULL; killed=killed->next){
			if(killed->id==links->miner_id){
				free_link(links, me);
				links=me->links;
				continue;
			}
		}
	}
*/	if(me->outbound!=NULL){
		for(links=me->outbound; links->next!=NULL; links=links->next){}
		for(; links!=NULL; links=links->prev){
			link = links->link;
//			fprintf(stderr, "sent block with height: %d to miner: %d\n", block->height, links->miner_id);
			
	/*		if(link==NULL){
				free_link(links, me);
				return;
			}
	*/
			if(links->new_comer != links->link->dest){
//			if(me->seed == true)
//				fprintf(stderr, "seed sent block with height: %d to miner: %d\n", block->height, links->miner_id);
				send_block(block, link);
			}
		}
	}
	if(me->inbound!=NULL){
		for(links=me->inbound; links->next!=NULL; links=links->next){}
		for(; links!=NULL; links=links->prev){
			link = links->link;
//			fprintf(stderr, "send block to miner: %d\n", links->miner_id);
		
/*			if(link==NULL){
				free_link(links, me);
				return;
			}
*/
			if(links->new_comer != links->link->dest){
//				if(me->seed == true)
//					fprintf(stderr, "seed sent block with height: %d to miner: %d\n", block->height, links->miner_id);
				send_block(block, link);
			}
		}
	}
//	fprintf(stderr, "propagated block\n"); //debug
}

struct blocks *mine_block(struct blocks *chain_head, unsigned int miner_id, struct miner *me){
	int				x,/* y,*/ /*times,*/ mined;
	double			y;
	struct block	*head, *current;
	struct blocks	*tmp;
	tmp = chain_head;
	if(tmp!=NULL){
		for(;tmp->next!=NULL; tmp=tmp->next){}
		me->blocks=tmp;
		current = tmp->block;
	}else{
		current = NULL;
	}
//	fprintf(stderr, "start mining on current: %p\n", current);
	mined = 0;
//	for(times = 0;times < 1;times++){
//		x = rand()%10;  // /(RAND_MAX);
//		y = rand()%10;  // /(RAND_MAX);
/*		if((x*x+y*y)>161*//*0.5 0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001*/
	x = rand()%601;
	y = (double )rand() /(RAND_MAX);
	if(me->new_chain==NULL&&(me->outbound!=NULL||me->inbound!=NULL)&&(x > 599 && y < me->hash_rate)){
//		fprintf(stdout,"mined block in %d times\n", times);
		mined			= 1;
		head			= malloc(sizeof(struct block));
		memset(head, 0, sizeof(struct block));
//		head->prev      = current;
		if(current==NULL){
			head->height	= 1;
			memset(head->hash, 0, SHA256_DIGEST_LENGTH);
		}
		else/* if(current->height!=0)*/{
			head->height	= current->height+1;
			memcpy(head->hash, SHA256((const unsigned char *)current, sizeof(struct block), 0), SHA256_DIGEST_LENGTH);
		}/*else{
			head->height	= 1;
			memset(head->prev, 0, SHA256_DIGEST_LENGTH);
		}*/
//		head->time              = ~~~
		head->miner_id	= miner_id;
		head->size		= sizeof(struct block);
		head->valid		= true;
	}
//	}
	if(mined){
//		fprintf(stderr, "will propagate block with height: %d\n", head->height); //debug
		tmp = add_block(head, tmp);
		propagate_block(head, me);
		add_block_record(me, head, tmp);
	}
//	fprintf(stderr, "finished mining for the turn\n"); //debug
	return tmp;
}

void request_block(unsigned int wanted_height, struct link *dest){
	int size;
	struct link *link;
	size = sizeof(unsigned int);
	link = dest;
	memcpy(&link->sbuf[0], "getblock", 8);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &wanted_height, size);
	send_msg(link->dest, (char *)link->sbuf, 16+size);
}
void get_blocks(struct link *dest, struct blocks *main_chain, struct blocks *new_chain){
	unsigned int	payload_size, sets, height, height2;
	struct blocks	*tmp, *tmp1;
//	fprintf(stderr, "will send getblocks\n");
	height = 0;
	height2 = 0;
	if(main_chain!=NULL)
		for(tmp=main_chain; tmp->next!=NULL; tmp=tmp->next){}
	if(new_chain!=NULL){
		for(tmp1=new_chain; tmp1->prev!=NULL; tmp1=tmp1->prev){}
#ifdef BLOCK_DEBUG
		fprintf(stderr, "getblocks: height = %d\n", tmp1->block->height);
#endif
		height	= (tmp1->block)->height;
	}
	if(main_chain==NULL){
		height2 = 1;
		memcpy(&dest->sbuf[0], "getblocks", 9);
		memcpy(&dest->sbuf[16], &height, sizeof(unsigned int));	
		memcpy(&dest->sbuf[16+sizeof(unsigned int)], &height2, sizeof(unsigned int));
		sets = 0;
		payload_size = sets*SHA256_DIGEST_LENGTH+ sizeof(unsigned int)+sizeof(unsigned int);
		memcpy(&dest->sbuf[12], &payload_size, 4);
		send_msg(dest->dest, (char *)dest->sbuf, 16+payload_size);
		return;
	}
//	if(!((tmp->block)->height < height)){
//		for(;tmp->prev!=NULL && (tmp->block)->height!=height-1; tmp=tmp->prev){}
//	}
//	height2 = (tmp->block)->height;
	height2 = 1;
//	if(height2 >= height-2)
//		height2=1;
	memcpy(&dest->sbuf[0], "getblocks", 9);
	memcpy(&dest->sbuf[16], &height, sizeof(unsigned int));
	memcpy(&dest->sbuf[16+sizeof(unsigned int)], &height2, sizeof(unsigned int));
	for(sets=0; tmp!=NULL && (16+(sets+1)*SHA256_DIGEST_LENGTH+sizeof(unsigned int)*2)<BUF_SIZE; sets++){
		memcpy(&dest->sbuf[16+sizeof(unsigned int)+sizeof(unsigned int)+sets*SHA256_DIGEST_LENGTH], &(tmp->block)->hash, SHA256_DIGEST_LENGTH);
		tmp=tmp->prev;
	}
	payload_size = sets*SHA256_DIGEST_LENGTH+ sizeof(unsigned int)+sizeof(unsigned int);
	memcpy(&dest->sbuf[12], &payload_size, 4);
	send_msg(dest->dest, (char *)dest->sbuf, 16+payload_size);
}

void send_blocks(struct link *from, struct blocks *main_chain, unsigned int height2, unsigned int height){
	struct blocks	*tmp;
	unsigned int	i;
//	for(tmp=main_chain; tmp->prev!=NULL; tmp=tmp->prev){}
/*	if(height)
		for(tmp=main_chain; (tmp->block)->height!=height && tmp->prev!=NULL; tmp=tmp->prev){}
	else
*/
#ifdef BLOCK_DEBUG
	fprintf(stderr, "send_blocks(): ");
#endif
	for(tmp=main_chain; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp->prev!=NULL && tmp->block->height!=height;tmp=tmp->prev){}
	for(i=0;((i*80)+16)<(BUF_SIZE-80)&&/*(tmp->block)->height!=height2-1*/tmp!=NULL;tmp=tmp->prev){
#ifdef BLOCK_DEBUG
		fprintf(stderr, " height %d, ", tmp->block->height);
#endif		
		send_block(tmp->block, from);
		i++;
	}
#ifdef BLOCK_DEBUG
	fprintf(stderr,"\n");
#endif
}

struct links *process_dns(struct link *new_comer, struct links *seeds, unsigned int dns_num){
	struct link				*link;
	struct links			*tmp;
	const struct msg_hdr	*hdr;
	link	= new_comer;
	hdr		= (struct msg_hdr*)(link->process_buf);
	if(strncmp(hdr->command, "dnsseed", 7)==0){
//		fprintf(stderr, "seed request received\n");
		tmp = seed_receive(link, seeds);
		if(tmp!=NULL)
			return tmp;
		else
			return seeds;
	}
	else if(strncmp(hdr->command, "dnsquery", 8)==0){
//		fprintf(stderr, "dnsquery received\n"); //debug
		dns_roundrobin(link,/* seeds,*/ dns_num);
		return seeds;
	}
	else{
		return seeds;
	}
}
int process_msg(struct link *new_comer,struct links *links, struct miner *me){
	bool				connect;//, connected;
	unsigned int		height, height2, i, miner_id, size, set, num_addr, payload_size;
	struct link			*link;//, *dest;
	struct links		/**tmp,*/ tmp2;
	const struct msg_hdr *hdr;
	struct block		*block;
	struct blocks		*blocks;
	struct killed		*killed;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
//	fprintf(stderr, "command: %s\n", hdr->command); //debug
	if(strncmp(hdr->command, "block", 5)==0){
		block=(struct block*)&link->process_buf[16];
#ifdef	DEBUG
		fprintf(stderr, "received block with height: %d from %d\n", block->height, links->miner_id); //debug
#endif	//DEBUG
#ifdef	BEHAVIOR
//	if(me->seed!=true)	
#endif	//BEHAVIOR
		me->blocks = process_new_blocks(block, me->blocks, me, link);
	}
	else if(strncmp(hdr->command, "getblocks", 9)==0){
//		fprintf(stderr, "getblocks received\n");
		memcpy(&height, &link->process_buf[16], sizeof(unsigned int));
		memcpy(&height2, &link->process_buf[16+sizeof(unsigned int)], sizeof(unsigned int));
//		fprintf(stderr, "getblocks received. height: %d, height2: %d\n", height, height2);
		if(me->blocks==NULL)
			return 1;
//		for(blocks=me->blocks; (blocks->block)->height!=height2 && blocks->prev!=NULL; blocks=blocks->prev){
//		}
//		for(set=0; blocks!=NULL; blocks=blocks->prev){
//			if(!memcmp(&link->process_buf[16+sizeof(unsigned int)+sizeof(unsigned int)+(set*SHA256_DIGEST_LENGTH)], (blocks->block)->hash, SHA256_DIGEST_LENGTH)){
//				height2=(blocks->block)->height;
//				break;
//			}
//			set++;
//		}
		if(me->blocks!=NULL){
//			fprintf(stderr, "connecting block found\n");
			send_blocks(link, me->blocks, /*height2*/1, height);
		}
//		if(blocks==NULL){
//			send_blocks(link, me->blocks, 1, height);
//		}
	}
	else if(strncmp(hdr->command, "getblock", 8)==0){
//		fprintf(stderr, "getblock received from: %d\n", links->miner_id);
		memcpy(&height, &link->process_buf[16], sizeof(unsigned int));
//		fprintf(stderr, "trying to find requested block: %d\n", height);
//		fprintf(stderr, "me->blocks = %p\n", me->blocks);
		if(me->blocks==NULL)
			return 0;
		for(blocks=me->blocks; blocks->next!=NULL; blocks=blocks->next){}
		block = blocks->block;
		for(block=blocks->block;block->height!=height && blocks!=NULL;blocks=blocks->prev){
			block=blocks->block;
			if(block->height==height){
				break;
			}
		}
		if(blocks==NULL)
			return 0;
		else{
			send_block(block, link);
//			fprintf(stderr, "sent block with height: %d to miner: %d\n", block->height, links->miner_id);
		}
	}
	else if(strncmp(hdr->command, "getaddr", 7)==0){
//		fprintf(stderr, "getaddr received\n");
		memcpy(&miner_id, &link->process_buf[16], sizeof(unsigned int));
		addr(link, me, miner_id);
		
	}
	else if(strncmp(hdr->command, "addr", 4)==0){
		memcpy(&payload_size, &link->process_buf[12], sizeof(unsigned int));
		size		= sizeof(struct link*);
		set			= size + sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int);
		num_addr    = payload_size/set;
//		fprintf(stderr, "addr num_addr = %d\n", num_addr);
//		hexDump("got addr message", &link->process_buf[16], (sizeof(struct link*)+sizeof(unsigned int)*3)*num_addr);
		if(num_addr == 0)
			return 0;
	//	connected = false;
		for(i=0; i<num_addr; i++){
			memcpy(&tmp2.miner_id, &link->process_buf[16+i*set], sizeof(unsigned int));
			memcpy(&tmp2.new_comer, &link->process_buf[16+i*set+sizeof(unsigned int)], size);
			memcpy(&tmp2.n_time, &link->process_buf[16+i*set+sizeof(unsigned int)+size], sizeof(unsigned int));
			memcpy(&tmp2.subnet, &link->process_buf[16+i*set+sizeof(unsigned int)+size+sizeof(unsigned int)], sizeof(unsigned int));
#ifdef DEBUG
			fprintf(stderr, "tmp2: miner_id = %d, new_comer = %p, n_time = %d, subnet = %d\n", tmp2.miner_id, tmp2.new_comer, tmp2.n_time, tmp2.subnet);
#endif
			connect		= true;
			if(tmp2.miner_id==me->miner_id){
				connect = false;
			}
			else{
/*				for(tmp=me->addrman; tmp->prev!=NULL; tmp=tmp->prev){}
				for(; tmp->next!=NULL; tmp=tmp->next){
					if(tmp->miner_id==miner_id){
						connect = false;
						break;
					}				
				}
*/			}
			if(dead!=NULL){
				for(killed=dead; killed!=NULL; killed=killed->next){
					if(killed->id==tmp2.miner_id){
						connect=false;
						break;
					}
				}
			}
			if(connect == true){
//				add_links(miner_id, dest, dest, me->addrman);
//				fprintf(stderr, "addr addrman_add_: tmp2.new_comer = %p, tmp2.miner_id = %d\n", tmp2.new_comer, tmp2.miner_id);
				addrman_add_(&me->addrman, &tmp2, links->link, 2*60*60);
			}
		}
/*		if(!connected){
			dns_query(&dns[rand()%NUM_DNS], &me->new_comer, me->miner_id);
		}
*/	return 1;
	}
	else if(strncmp(hdr->command, "verack", 6)==0){
//		link->dest = (struct link*)payload;
		memcpy(&link->dest, &link->process_buf[16], sizeof(struct link*));
#ifdef VER_DEBUG
		fprintf(stderr, "verack addrman_add_: links->new_comer = %p, links->miner_id = %d, &links->link->dest %p\n", links->new_comer, links->miner_id, &links->link->dest);
#endif
		addrman_add_(&me->addrman, links, links->new_comer, 0);
		addrman_good(&me->addrman, links->new_comer, sim_time);

		if(me->addrman.v_random_size < THOUSAND){
			getaddr(link, me->miner_id);
		}	
		if(me->blocks==NULL)
			get_blocks(links->link, me->blocks, me->new_chain);
//		fprintf(stderr, "received verack with link: %p\n", link->dest);
	}
	else if(strncmp(hdr->command, "nat", 3)==0){
#ifdef DEBUG
		fprintf(stderr, "nat received\n");
#endif
		free_link(links, me);
		
//		me->n_outbound--;
		return -1;
	}
	return 0;
}

void process_new(struct link *new_comer, struct miner *me){
    unsigned int        size, payload_size, dest_id, miner_id, subnet;
    struct link         *link, *dest;
	struct links		*tmp;
	struct msg_hdr *hdr;
    struct blocks       *blocks;
	struct killed		*killed;

	size = sizeof(struct link*);
    link = new_comer;
    hdr = (struct msg_hdr*)(link->process_buf);
	memcpy(&payload_size, &link->process_buf[12], 4);
//    fprintf(stderr, "check command\n"); //debug
//	hexDump("Message received", link->process_buf, 16+hdr->message_size);
    if(strncmp(hdr->command, "roundrobin", 10)==0){
		memcpy(&dest, &link->process_buf[16], size);
		memcpy(&dest_id, &link->process_buf[16+size], sizeof(unsigned int));
		memcpy(&subnet, &link->process_buf[16+size+sizeof(unsigned int)], sizeof(unsigned int));
		if(dest_id==me->miner_id){
			return;
		}
		if(dead!=NULL)
			for(killed=dead; killed!=NULL; killed=killed->next){
				if(killed->id == dest_id)
					return;
			}
// already connected
		if(me->outbound!=NULL){
			for(tmp=me->outbound; tmp->next!=NULL; tmp=tmp->next){}
			for(; tmp!=NULL; tmp=tmp->prev){
				if(tmp->miner_id==dest_id){
					return;
				}
			}
		}
		if(me->inbound!=NULL){
			for(tmp=me->inbound; tmp->next!=NULL; tmp=tmp->next){}
			for(; tmp!=NULL; tmp=tmp->prev){
				if(tmp->miner_id==dest_id){
//					return me->inbound;
					return;
				}
			}
		}
//		me->neighbor++;
		me->n_outbound++;
		version(me->miner_id, me->subnet, dest_id, dest, &me->new_comer, me, subnet);
	}
	else if(strncmp(hdr->command, "version", 7)==0){
		bool links_found=false;
		struct links *links;
//		fprintf(stderr, "version received\n");
//		if(me->one_way==NOT_NAT && me->neighbor < me->max){
		memcpy(&dest, &new_comer->process_buf[16], size);
#ifdef VER_DEBUG
		fprintf(stderr, "version msg received: dest=  %p\n", dest);
#endif
		if(me->outbound!=NULL&&links_found!=true){
			for(links=me->outbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=links->prev){
				if(dest==links->link->dest){
					links_found=true;
					break;
				}
			}
		}
		if(me->inbound!=NULL&&links_found!=true){
			for(links=me->inbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=links->prev){
				if(dest==links->link->dest){
					links_found=true;
					break;
				}
			}
		}
		if(((me->one_way==NOT_NAT && me->n_inbound <= N_MAX_CONNECTIONS - MAX_OUTBOUND_CONNECTIONS)||me->seed==true)&&links_found==false){
			me->n_inbound++;
			tmp = me->inbound;// me->links;
			me->inbound = verack(new_comer, me->inbound, me);
			if(tmp==me->inbound){
//				me->neighbor--;
				me->n_inbound--;
				return;
			}
			me->inbound->n_time=sim_time;
			addrman_add_(&me->addrman, me->inbound, me->inbound->new_comer, 0);
			addrman_good(&me->addrman, me->inbound->new_comer, sim_time);
			if(me->blocks!=NULL){

				for(blocks=me->blocks; blocks->next!=NULL; blocks=blocks->next){}
				send_block(blocks->block, /*(me->links)->link*/me->inbound->link);	
//				send_blocks(me->inbound->link, blocks, /*height2*/1, 1);
			}
			return;
		} 
		else if(links_found!=true){
			memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
			nat(new_comer, miner_id, me->inbound, me);
			return;
		}
    }
    return;
}

void check_subnet(struct links *outbound, struct caddrinfo *addr, bool *subnet_found){
	struct links *links=outbound;
	for(links=outbound; links->next!=NULL; links=links->next){}
	for(; links!=NULL; links=links->prev){
		if(links->subnet==addr->subnet||links->new_comer==addr->new_comer||links->miner_id==addr->miner_id){
			*subnet_found=true;
			break;
		}
	}
}

void make_random_connection(struct threads *threads){
	struct threads *tmp, *tmp2, *init;
	struct miner *s, *d;
	int		dest, i, seed;
	for(tmp=threads; tmp->prev!=NULL; tmp=tmp->prev){}
#ifndef MULTI
#ifdef	BAD_NODES
	for(i=0; i<BAD_NODES; i++){
		tmp=tmp->next;
	}
#endif	//BAD_NODES
#endif	//MULTI
	init=tmp;
	for(; tmp!=NULL; tmp=tmp->next){
		seed = rand()%SEED_NUM;
		dest = rand()%TOTAL_NODES;
		tmp2=init;
		for(i=0; i<dest && tmp2->next!=NULL; i++){
			tmp2=tmp2->next;
		}
		s = tmp->miner;
		s->boot=false;
		if(tmp!=tmp2){
			d = tmp2->miner;
			version(s->miner_id, s->subnet, d->miner_id, &d->new_comer, &s->new_comer, s, d->subnet);
		}
		version(s->miner_id, s->subnet, seeds[seed]->miner_id, &seeds[seed]->new_comer, &s->new_comer, s, seeds[seed]->subnet);
	}
}
#endif
