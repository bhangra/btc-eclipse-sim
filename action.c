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

#include"block.c"
#include"thread.c"
#include"connection.c"
#include"proto-node.h"


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

void dns_seed(unsigned int my_id, struct dns *dns, struct link *new_comer){
    struct links	*tmp, *new;
    struct link		*link;
    unsigned int	size, payload_size;
    size			= sizeof(struct link*);
	link			= new_comer; 
    link->dest		= &dns->new_comer;
	payload_size	= size + sizeof(unsigned int);
    memcpy(&link->sbuf[0], "dnsseed", 7);
    memcpy(&link->sbuf[12], &payload_size, sizeof(unsigned int));
    memcpy(&link->sbuf[16], &link, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
    send_msg(&dns->new_comer, link->sbuf, 16+payload_size);
}
struct links *seed_receive(struct link *new_comer, struct links *seeds){
	unsigned int	miner_id, size;
	struct link		*link;
	struct links	*new;
	size = sizeof(struct link*);
	memcpy(&link, &new_comer->process_buf[16], size);
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	fprintf(stderr, "seed link: %p, id: %d\n", link, miner_id);
//	fprintf(stderr, "seed_receive(): will add_links()\n");
	return add_links(miner_id, link, link, seeds);
}
void dns_query(struct dns *dns, struct link *new_comer, unsigned int my_id){
	unsigned int	size, payload_size, miner_id;
	struct link		*link;
	struct links	*tmp;
	size		= sizeof(struct link*);
	payload_size= size+sizeof(unsigned int);
	miner_id	= my_id;
	link		= new_comer;
	link->dest	= &(dns->new_comer);
	memcpy(&link->sbuf[0], "dnsquery", 8);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &new_comer, size);
	memcpy(&link->sbuf[16+size], &miner_id, sizeof(unsigned int));
	
	send_msg(&dns->new_comer, link->sbuf, 16+payload_size);  
}

void dns_roundrobin(struct link *new_comer, struct links *seeds){
	unsigned int	i, j, num_seeds, size, payload_size, dest_id, random;
	struct link		*link;
	struct links	*tmp, *head;
	size			= sizeof(struct link*);
	payload_size	= size + sizeof(unsigned int);
	link 			= new_comer;

//	srand((unsigned)time(NULL));

	memcpy(&dest_id, &link->process_buf[16+size], sizeof(unsigned int));
	for(tmp=seeds;tmp->prev!=NULL;tmp=tmp->prev){}
	head=tmp;
	for(i=1; tmp->next!=NULL; i++){
		tmp=tmp->next;
	}
	num_seeds=i;
	for(; ;){
		i=num_seeds;
		random=rand();
		fprintf(stderr, "i = %d, random = %d\n", i, random);
		if(num_seeds<=0){
			fprintf(stderr, "no seeds\n");
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
			for(i=0; i<j; tmp=tmp->next/*i++*/){
				i++;//	tmp = tmp->next;
			}
			if(tmp->miner_id != dest_id){
				break;
			}
		}
	}
	fprintf(stderr, "seed: %d %p\n", tmp->miner_id, tmp->new_comer);
	memcpy(&link->dest, &link->process_buf[16], size);
	memcpy(&link->sbuf[0], "roundrobin", 10);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &tmp->new_comer, size);
	memcpy(&link->sbuf[16+size], &tmp->miner_id, sizeof(unsigned int));
	send_msg(link->dest, link->sbuf, 16+payload_size);
	fprintf(stderr, "sent DNS round robin\n");
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
	send_msg(link->dest, link->sbuf, 16+size);
}

void addr(struct link *dest, struct miner *me, unsigned int dest_id){
	unsigned int	size, set;
	int				i;
	struct link		*link, *tmp;
	struct links 	*links;
	link	= dest;
	size	= 0;
	set		= sizeof(struct link*) + sizeof(unsigned int);
	memcpy(&link->sbuf[0], "addr", 4);
	for(links=me->links; links->prev!=NULL; links=links->prev){}
	for(i=0; links!=NULL; i++){
		if(links->miner_id!=dest_id){
//			link=links->link;
			fprintf(stderr, "adding to addr dest: %p, id: %d\n", links->new_comer, links->miner_id);
			memcpy(&link->sbuf[16+(i*set)], &links->miner_id, sizeof(unsigned int));	
			memcpy(&link->sbuf[16+(i*set)+sizeof(unsigned int)], &links->new_comer, sizeof(struct link*));
		}
		else{
			i=i-1;
		}
		links=links->next;
	}
	size=i*set;
	memcpy(&link->sbuf[12], &size, 4);
	send_msg(link->dest, link->sbuf, 16+(set*i));
}
//dest is desination's new_comer
struct links *version(unsigned int my_id, unsigned int dest_id, struct link *dest, struct link *new_comer, struct links *links){
	unsigned int miner_id;
	struct links *new;
	struct link *link, *tmp;
	unsigned int size, payload_size;
//	fprintf(stderr, "will send version msg\n"); //debug
//	fprintf(stderr, "version to link: %p id: \n", dest, dest_id); //debug
	size = sizeof(struct link*);
	payload_size = size+sizeof(unsigned int)+sizeof(struct link*);
	new = add_links(dest_id, dest, dest,  links);
	link = new->link;
	tmp = new_comer;
	memcpy(&link->sbuf[0], "version", 7);
	memcpy(&link->sbuf[12], &payload_size, 4);
	memcpy(&link->sbuf[16], &link, size);
	memcpy(&link->sbuf[16+size], &my_id, sizeof(unsigned int));
	memcpy(&link->sbuf[16+size+sizeof(unsigned int)], &tmp, sizeof(struct link*));
	send_msg(dest, link->sbuf, 16+payload_size);
	fprintf(stderr, "sent version to dest: %p, id: %d with mylink: %p\n", link->dest, new->miner_id, link); //debug
	return new;
}

struct links *verack(struct link *new_comer, struct links *links){
	unsigned int miner_id;
	struct links *tmp, *new;
	struct link *link, *dest, *dest_new_comer;
	unsigned int size;
//	fprintf(stderr, "will send verack msg\n"); //debug
	size = sizeof(struct link*);
//	dest		= (struct link*)&new_comer->process_buf[16];

	memcpy(&dest, &new_comer->process_buf[16], size);
	memcpy(&miner_id, &new_comer->process_buf[16+size], sizeof(unsigned int));
	memcpy(&dest_new_comer, &new_comer->process_buf[16+size+sizeof(unsigned int)], sizeof(struct link*));
	tmp			= add_links(miner_id, dest, dest_new_comer, links);
	link		= tmp->link;
	fprintf(stderr, "verack to dest: %p, id: %d with mylink: %p\n", dest, miner_id, link);
	memcpy(&link->sbuf[0], "verack", 6);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &link, size);
	send_msg(link->dest, link->sbuf, 16+size);
//	fprintf(stderr, "sent verack to: %d\n", tmp->miner_id); //debug
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
	send_msg(link->dest, link->sbuf, 16+size);
}

void propagate_block(struct block *block, struct miner *me){
	struct link		*link;
	struct links	*links;
//	fprintf(stderr, "me->links = %p\n", me->links);
	if(me->links==NULL)
		return;
	for(links=me->links; links->prev!=NULL;links=links->prev){}
	for(; links!=NULL; links=links->next){
		link = links->link;
		fprintf(stderr, "sent block with height: %d to miner: %d\n", block->height, links->miner_id);
		send_block(block, link);
	}
	fprintf(stderr, "propagated block\n"); //debug
}

struct blocks *mine_block(struct blocks *chain_head, unsigned int miner_id, struct miner *me){
	int				x, y, times, mined;
	struct block	*head, *current;
	struct blocks	*tmp;
	struct link		*link;
	struct links	*links;
	tmp = chain_head;
	if(tmp!=NULL){
		for(;tmp->next!=NULL; tmp=tmp->next){}
		current = tmp->block;
	}else{
		current = NULL;
	}
	fprintf(stderr, "start mining on current: %p\n", current);
	mined = 0;
	for(times = 0;times < 1/*000*/;times++){
		x = rand()%5;  // /(RAND_MAX);
		y = rand()%5;  // /(RAND_MAX);
		if((x*x+y*y)>30/*0.5 0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001*/){
			fprintf(stdout,"mined block in %d times\n", times);
			mined			= 1;
			head			= malloc(sizeof(struct block));
			memset(head, 0, sizeof(struct block));
//			head->prev      = current;
			if(current==NULL){
				head->height	= 1;
				memset(head->hash, 0, SHA256_DIGEST_LENGTH);
			}
			else/* if(current->height!=0)*/{
				head->height	= current->height+1;
				memcpy(head->hash, SHA256((char *)current, sizeof(struct block), 0), SHA256_DIGEST_LENGTH);
			}/*else{
				head->height	= 1;
				memset(head->prev, 0, SHA256_DIGEST_LENGTH);
			}*/
//			head->time              = ~~~
			head->miner_id	= miner_id;
			head->size		= sizeof(struct block);
			head->valid		= true;
			break;
		}
	}
	if(mined){
		fprintf(stderr, "will propagate block with height: %d\n", head->height); //debug
		tmp = add_block(head, tmp);
		propagate_block(head, me);
	}
	fprintf(stderr, "finished mining for the turn\n"); //debug
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
	send_msg(link->dest, link->sbuf, 16+size);
}

struct links *process_dns(struct link *new_comer, struct links *seeds){
	char 					*payload;
	unsigned int			dest_id;
	struct link				*link;
	struct links			*tmp;
	const struct msg_hdr	*hdr;
	link	= new_comer;
	hdr		= (struct msg_hdr*)(link->process_buf);
	payload	= (char *)(hdr+sizeof(struct msg_hdr));
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
int process_msg(struct link *new_comer,struct links *links, struct miner *me){
	bool				connect, connected;
	char				*payload;
	unsigned int		height, i, miner_id, size, set, num_addr, payload_size;
	struct link			*link, *dest;
	struct links		*tmp;
	const struct msg_hdr *hdr;
	struct block		*block;
	struct blocks		*blocks;

	link = links->link;
	hdr = (struct msg_hdr*)(link->process_buf);
	payload = (char *)(hdr +sizeof(struct msg_hdr));
	fprintf(stderr, "command: %s\n", hdr->command); //debug
	if(strncmp(hdr->command, "block", 5)==0){
		block=(struct block*)&link->process_buf[16];
		fprintf(stderr, "received block with height: %d from %d\n", block->height, links->miner_id); //debug
		me->blocks = process_new_blocks(block, me->blocks, me, link);
	}
/*	else if(strncmp(hdr->command, "newhead", 7)==0){

	}
*/
	else if(strncmp(hdr->command, "getblock", 8)==0){
		fprintf(stderr, "getblock received from: %d\n", links->miner_id);
		memcpy(&height, &link->process_buf[16], sizeof(unsigned int));
		fprintf(stderr, "trying to find requested block: %d\n", height);
		fprintf(stderr, "me->blocks = %p\n", me->blocks);
		if(me->blocks==NULL)
			return;
		for(blocks=me->blocks; blocks->next!=NULL; blocks=blocks->next){}
		block = blocks->block;
		for(block=blocks->block;block->height!=height && blocks!=NULL;blocks=blocks->prev){
			block=blocks->block;
			if(block->height==height){
				break;
			}
		}
		if(blocks==NULL)
			return;
		else{
			send_block(block, link);
			fprintf(stderr, "sent block with height: %d to miner: %d\n", block->height, links->miner_id);
		}
	}
	else if(strncmp(hdr->command, "getaddr", 7)==0){
		fprintf(stderr, "getaddr received\n");
		memcpy(&miner_id, &link->process_buf[16], sizeof(unsigned int));
		addr(link, me, miner_id);
	}
	else if(strncmp(hdr->command, "addr", 4)==0){
		memcpy(&payload_size, &link->process_buf[12], sizeof(unsigned int));
//		num_addr	= (hdr->message_size/(size+sizeof(unsigned int)));
		size		= sizeof(struct link*);
		set			= size + sizeof(unsigned int);
		num_addr    = payload_size/set;
		fprintf(stderr, "num_addr = %d\n", num_addr);
		if(num_addr == 0)
			me->boot = true;
		for(i=0; i<num_addr; i++){
			memcpy(&miner_id, &link->process_buf[16+i*set], sizeof(unsigned int));
			memcpy(&dest, &link->process_buf[16+i*set+sizeof(unsigned int)], size);
			connect		= true;
			connected	= false;
			for(tmp=me->links; tmp->next!=NULL; tmp=tmp->next){
				if(tmp->miner_id==miner_id){
					connect = false;
					break;
				}				
			}
			if(connect){
				connected = true;
				fprintf(stderr, "will send version to dest: %p, id: %d\n", dest, miner_id);
				me->links = version(me->miner_id, miner_id, dest, &me->new_comer, me->links);
			}
		}
		if(!connected){
			dns_query(&dns[rand()%6], &me->new_comer, me->miner_id);
		}
	}
	else if(strncmp(hdr->command, "verack", 6)==0){
//		link->dest = (struct link*)payload;
		memcpy(&link->dest, &link->process_buf[16], sizeof(struct link*));
		fprintf(stderr, "received verack with link: %p\n", link->dest);
	}
	return 0;
}

struct links *process_new(struct link *new_comer, struct miner *me){
    char                *payload;
    unsigned int        height, i, miner_id, size, payload_size;
    struct link         *link, *dest;
	struct msg_hdr *hdr;
    struct block        *block;
    struct blocks       *blocks;

	size = sizeof(struct link*);
    link = new_comer;
    hdr = (struct msg_hdr*)(link->process_buf);
    payload = (char *)(hdr +sizeof(struct msg_hdr));
	memcpy(&payload_size, &link->process_buf[12], 4);
//    fprintf(stderr, "check command\n"); //debug
//	hexDump("Message received", link->process_buf, 16+hdr->message_size);
    if(strncmp(hdr->command, "roundrobin", 10)==0){
//		memcpy(&link, &link->process_buf[16], size);
		memcpy(&dest, &link->process_buf[16], size);
		memcpy(&miner_id, &link->process_buf[16+size], sizeof(unsigned int));
		if(miner_id==me->miner_id){
			fprintf(stderr, "will not send version to me\n");
			return NULL;
		}
		return version(me->miner_id, miner_id, dest, &me->new_comer, me->links);
    }
	else if(strncmp(hdr->command, "version", 7)==0){
		fprintf(stderr, "version received\n");
        return verack(new_comer, me->links);
    }
    return NULL;
}


#endif
