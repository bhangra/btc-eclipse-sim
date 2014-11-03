#ifndef ACTION_C
#define ACTION_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"thread.c"
#include"connection.c"
#include"proto-node.h"


void dns_seed(struct dns *dns, struct link *new_comer){
    struct links *tmp, *new;
    struct link *link;
    int size;
    size = sizeof(struct link*);
	link = new_comer; 
    link->dest = &dns->new_comer;
    memcpy(&link->sbuf[0], "dnsseed", 7);
    memcpy(&link->sbuf[12], &size, 4);
    memcpy(&link->sbuf[16], &new_comer, size);
    send_msg(&dns->new_comer, link->sbuf, 16+size);
}
void seed_receive(struct link *new_comer, struct links *seeds){
	struct link		*link;
	struct links	*tmp, *new;

	for(tmp = seeds; tmp->next!=NULL;tmp=tmp->next){
		if(0==(memcmp(&((tmp->link)->dest), &new_comer->process_buf[16], sizeof(struct link*))))
			return;
	}
	new			= malloc(sizeof(struct links));
	link		= malloc(sizeof(struct link));
	tmp->next	= new;
	new->prev	= tmp;
	new->link	= link;
	memcpy(&link->dest, &new_comer->process_buf[16], sizeof(struct link*));
}
void dns_query(struct dns *dns, struct link *new_comer){
	int size;
	struct link *link;
	struct links *tmp;
	size = sizeof(struct link*);
	link = new_comer;
	link->dest = &(dns->new_comer);
	memcpy(&link->sbuf[0], "dnsquery", 8);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &new_comer, size);
	send_msg(&dns->new_comer, link->sbuf, 16+size);  
}
void dns_roundrobin(struct link *new_comer, struct links *seeds){
	int i, j, size;
	struct link *link;
	struct links *tmp, *head;
	size = sizeof(struct link*);
	link = new_comer;
	for(tmp=seeds;tmp->prev!=NULL;tmp=tmp->prev){;}
	head=tmp;
	for(i=1; tmp->next!=NULL; i++){
		tmp=tmp->next;
	}
	j=rand()%i;
	tmp = head;
	for(i=0; i<j; i++){
		tmp = tmp->next;
	}
	memcpy(&link->dest, &(link->process_buf[16]), size);
	memcpy(&link->sbuf[0], "roundrobin", 10);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], tmp->link, size);
	send_msg(link->dest, link->sbuf, 16+size);
}
void version(struct link *dest, struct links *links){
	struct links *tmp, *new;
	struct link *link;
	int size;
	size = sizeof(struct link*);
	tmp = links; //might put func to get tail
	new = malloc(sizeof(struct links));
	new->link = malloc(sizeof(struct links));
	link = new->link;
	tmp->next = new;
	new->prev = tmp;
	memcpy(&link->sbuf[0], "version", 7);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &link, size);
	send_msg(dest, link->sbuf, 16+size);
}
void verack(struct links *links){
	struct links *tmp, *new;
	struct link *link;
	int size;
	tmp = links; //might put func to get tail
	size = sizeof(struct link*);
	new = malloc(sizeof(struct links));
	new->link = malloc(sizeof(struct link));
	link = new->link;
	tmp->next = new;
	new->prev = tmp;
	memcpy(&link->dest, &link->process_buf[16], size);
	memcpy(&link->sbuf[0], "verack", 6);
	memcpy(&link->sbuf[12], &size, 4);
	memcpy(&link->sbuf[16], &link, size);
	send_msg(link->dest, link->sbuf, 16+size);
}
//chain_head = add_block(&new_block, &chain_head);
struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	tmp->block			= block;
	chain_head->next 	= tmp;
	tmp->prev 			= chain_head;
	return tmp;
}
struct blocks *mine_block(struct blocks *chain_head, unsigned int miner_id){
	int x, y, times, mined;
	struct block *head, *current;
	struct blocks *tmp;
	tmp = chain_head;
	if(tmp!=NULL){
		current = tmp->block;
	}else{
		current = NULL;
	}
	mined = 0;
	for(times = 0;times < 1000;times++){
		x = rand()/(RAND_MAX);
		y = rand()/(RAND_MAX);
		if((x*x+y*y)>0.5){
			fprintf(stderr,"mined block in %d times\n", times);
			mined = 1;
			head 			= malloc(sizeof(struct block));
			head->prev		= current;
			if(current->height!=0){
				head->height	= current->height++;
			}else{
				head->height	= 1;
			}
//			head->time              = ~~~
			head->miner_id	= miner_id;
			head->size		= sizeof(struct block);
			head->valid		= true;
			break;
		}
	}
	if(mined){
		tmp = add_block(head, tmp);
	}
	return tmp;
}
void get_block(struct link *link){

}
void request_block(struct block *wanted, struct link *dest){
	
}
int verify_block(struct block *new_block, struct blocks *chain_head){

	struct block *tmp;
	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		return -1;
	}
	else if(new_block->prev != (tmp)){
		return 0;
	}
	return 1;

}
struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct link *from){
	int rerun;
	int validity;
	struct blocks *tmp;
	tmp = chain_head;
	for(rerun = 1; rerun==1;){
		rerun = 0;
		for(;;){
			validity = verify_block(block, tmp);
			if(validity==1){
				add_block(block, chain_head);
			}
			else if(validity==0){
				request_block(block->prev, from);
				tmp = tmp->prev;
			}
			else if(validity==-1){
				return chain_head;
			}
		}
	}
	return chain_head;
}
int process_dns(struct link *new_comer, struct links *seeds){
	char * payload;
	struct link *link;
	const struct msg_hdr *hdr;
	link	= new_comer;
	hdr		= (struct msg_hdr*)(link->process_buf);
	payload	= (char *)(hdr+sizeof(struct msg_hdr));
	if(strncmp(hdr->command, "dnsseed", 7)==0){
		seed_receive(link, seeds);
		return 1;
	}
	else if(strncmp(hdr->command, "dnsquery", 8)==0){
		dns_roundrobin(link, seeds);
		return 1;
	}
	else{
		return 0;
	}
}
int process_msg(struct links *links){
	char *payload;
	struct link *link;
	link = links->link;
	const struct msg_hdr *hdr;
	hdr = (struct msg_hdr*)(link->process_buf);
	payload = (char *)(hdr +sizeof(struct msg_hdr));
	if(strncmp(hdr->command, "block", 5)==0){
		
	}
	else if(strncmp(hdr->command, "newhead", 7)==0){

	}
	else if(strncmp(hdr->command, "getblock", 8)==0){

	}
	else if(strncmp(hdr->command, "roundrobin", 10)==0){
		version((struct link*)payload, links);
	}
	else if(strncmp(hdr->command, "getaddr", 7)==0){
		
	}
	else if(strncmp(hdr->command, "addr", 4)==0){

	}
	else if(strncmp(hdr->command, "version", 6)==0){
		verack(links);
	}
	else if(strncmp(hdr->command, "verack", 6)==0){
		link->dest = (struct link*)payload;
//		struct link *tmp;
//		tmp = link;
//		memcpy(&tmp->dest, &tmp->process_buf[16], sizeof(struct link*));
	}
	return 0;
}

#endif
