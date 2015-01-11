#ifndef CONNECTION_C
#define CONNECTION_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<unistd.h>
#include<signal.h>
#include<pthread.h>
#include<assert.h>

#include"connection.h"
#include"proto-node.h"
#include"thread.h"
#include"global.c"

#define HDR_SIZE	16
void hexDump (char *desc, void *addr, int len);



void free_link(struct links *will_remove, struct miner *miner/*struct miner *miner*/){
	struct links *after, *before, *tmp;
	bool found=false;
	if(will_remove==NULL){
		return;
	}
	if(miner->outbound!=NULL){
		for(tmp=miner->outbound; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if(tmp==will_remove){
				miner->n_outbound--;
				found = true;
				break;
			}
		}
	}
	if(miner->inbound!=NULL&&!found){
		for(tmp=miner->inbound; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if(tmp==will_remove){
				miner->n_inbound--;
				found = true;
				break;
			}
		}
	}
	after	= will_remove->next;
	before	= will_remove->prev;
	if(will_remove!=NULL){
		if(will_remove->link!=NULL)
			free(will_remove->link);
		will_remove->link = NULL;
//		fprintf(stderr, "freed links: %p, link: %p\n", will_remove, will_remove->link);
		free(will_remove);
//		will_remove = NULL;
	}
//	miner->neighbor--;
	if(after!=NULL)
		after->prev=before;
	if(before!=NULL)
		before->next=after;
		
	if(will_remove==miner->outbound){
//		miner->n_outbound--;
		if(after!=NULL)
			miner->outbound=after;
		else 
			miner->outbound=before;
	}
	if(will_remove==miner->inbound){
//		miner->n_inbound--;
		if(after!=NULL)
			miner->inbound=after;
		else
			miner->inbound=before;
	}
}

void free_links(struct threads *will_kill){
	unsigned int kill_id, i;
	struct threads  *tmp;
	struct miner    *miner;
	struct links    *links, *prev;
#ifdef BAD_NODES
	struct links	 *next;
#endif	
	kill_id = (will_kill->miner)->miner_id;
	
	for(tmp = will_kill; tmp->next!=NULL;tmp=tmp->next){}
	for(miner=tmp->miner; tmp!=NULL; tmp=tmp->prev){
		miner=tmp->miner;
		if(miner->outbound!=NULL){
			for(links=miner->outbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=prev){
				prev = links->prev;
				if(links->miner_id==kill_id){
					free_link(links, miner);
//					links=miner->outbound;
					break;
//					miner->n_outbound--;
				}
			}
		}
		if(miner->inbound!=NULL){
			for(links=miner->inbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=prev){
				prev = links->prev;
				if(links->miner_id==kill_id){
					free_link(links, miner);
//					links=miner->inbound;
					break;
//					miner->n_inbound--;
				}
			}
		}
	}
	for(i=0; i<SEED_NUM; i++){
		miner=&seeds[i];
		if(miner->outbound!=NULL){
			for(links=miner->outbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=prev){
				prev = links->prev;
				if(links->miner_id==kill_id){
					free_link(links, miner);
//					links=miner->outbound;
					break;
//					miner->n_outbound--;
				}
			}
		}
		if(miner->inbound!=NULL){
			for(links=miner->inbound; links->next!=NULL; links=links->next){}
			for(; links!=NULL; links=prev){
				prev = links->prev;
				if(links->miner_id==kill_id){
					free_link(links, miner);
//					links=miner->inbound;
					break;
//					miner->n_inbound--;
				}
			}
		}
	}
#ifdef BAD_NODES
	for(links=bad_links; links!=NULL; links=next){
		prev = links->prev;
		next = links->next;
		if(links->miner_id==kill_id){
			if(links==bad_links)
				bad_links=next;
			free(links->link);
			free(links);
			if(next!=NULL)
				next->prev=prev;
			if(prev!=NULL)
				prev->next=next;
		}
	}
#endif
}
void free_dns_rec(struct threads *will_kill){
	unsigned int 	kill_id, i;
	struct links	*tmp, *next, *prev;
	kill_id = (will_kill->miner)->miner_id;
	for(i=0; i<NUM_DNS; i++){
		if(dns[i].seeds==NULL)
			continue;
		for(tmp = dns[i].seeds; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if(tmp->miner_id == kill_id){
				next = tmp->next;
				prev = tmp->prev;
				free(tmp->link);
				free(tmp);
				if(next!=NULL)
					next->prev = prev;
				if(prev!=NULL)
					prev->next = next;
				if(dns[i].seeds == tmp){
					if(next!=NULL)
						dns[i].seeds = next;
					else if(prev!=NULL)
						dns[i].seeds = prev;
					else
						dns[i].seeds = NULL;
				}
				break;
			}
		}
	}
}

struct links *add_links(unsigned int miner_id, struct link *dest, struct link *new_comer, struct links *links){
	struct links *tmp, *new;
	for(tmp=links;; tmp=tmp->next){ //get tail
//		fprintf(stderr, "*links = %p\n", tmp); //debug
		if(tmp==NULL)
			break;
//#ifdef DEBUG
//			fprintf(stderr, "tmp->miner_id == %d\n", tmp->miner_id);
//#endif
		if(miner_id==tmp->miner_id){ //already connected 
#ifdef DEBUG
			fprintf(stderr, "already connected\n");
#endif
//			return links;
		}
		if(tmp->next == NULL){
			break;
		}
	}
	new			= malloc(sizeof(struct links));
/*	if(new==NULL){
		perror("malloc");
		exit(-1);
	}
*/	memset(new, 0, sizeof(struct links));
	new->link	= malloc(sizeof(struct link));
/*	if(new->link==NULL){
		perror("malloc");
		exit(-1);
	}*/
#ifdef MEM_DEBUG
	fprintf(stderr, "new->link = %p\n", new->link);
#endif
	memset(new->link, 0, sizeof(struct link));

	new->next	= NULL;
	new->prev	= tmp;
	if(tmp!=NULL)
		tmp->next	= new;
	new->miner_id		= miner_id;
	new->new_comer		= new_comer;
	(new->link)->dest	= dest;
//	fprintf(stderr, "added link\n");
	return new;
}

int send_msg(struct link *dest, char *message, unsigned int msg_size){
//	hexDump("sending msg", message, msg_size);
	int 			tmp;
	unsigned int 	pos, over_size;
//#ifdef MEM_DEBUG
//	fprintf(stderr, "sending msg_size: %d type: %s &dest->write_pos: %p &dest->buf[0]: %p\n", msg_size, message, &dest->write_pos, &dest->buf[0]); //debug
//#endif
#ifdef ASSERT
	assert(msg_size < BUF_SIZE );
	assert(dest->write_pos<BUF_SIZE);
#endif
/*
	if(dest->write_pos > BUF_SIZE){
		dest->read_pos = 0;
		dest->write_pos= 0;
		memset(dest->buf, 0, sizeof(dest->buf));
	}
*/
#ifdef ASSERT
	assert(dest->write_pos < BUF_SIZE);
#endif
	pos			= dest->write_pos;
	tmp			= pos + msg_size;
	if(pos+msg_size >=BUF_SIZE)
		tmp			= pos+msg_size-BUF_SIZE;
#ifdef MEM_DEBUG
	fprintf(stderr, "dest->num_msg = %d, dest->write_pos = %d, dest->read_pos = %d\n", dest->num_msg, pos, dest->read_pos);
#endif
	if(pos < dest->read_pos && (pos+msg_size > dest->read_pos) && pos+msg_size <BUF_SIZE){
#ifdef DEBUG
		fprintf(stderr, "catched up to read_pos\n");
#endif
		return 0;
	}
	if((pos < dest->read_pos) && (tmp < dest->read_pos) && pos+msg_size>BUF_SIZE){
#ifdef DEBUG
		fprintf(stderr, "rounded around buffer, catching up to read_pos. tmp = %d\n", tmp);
#endif
		return 0;
	}
	if(pos > dest->read_pos && tmp >= dest->read_pos && pos+msg_size > BUF_SIZE){
#ifdef DEBUG
		fprintf(stderr, "was above read pos & rounded around buffer, going above read pos again\n");
#endif
		return 0;
	}
#ifdef MULTI
	pthread_mutex_lock((pthread_mutex_t *)&dest->rcv_mutex);
#endif
	if(pos+msg_size < BUF_SIZE){
		memcpy(&dest->buf[pos], message, msg_size);
		dest->write_pos += msg_size;
	}
	else{
		over_size = (pos+msg_size) - (BUF_SIZE);
		memcpy(&dest->buf[pos], message, (msg_size-over_size));
		memcpy(&dest->buf[0], &message[msg_size-over_size], over_size);
		dest->write_pos = over_size;
	}
	dest->num_msg += 1;
#ifdef MULTI
	pthread_mutex_unlock((pthread_mutex_t *)&dest->rcv_mutex);
#endif
	memset(message, 0, BUF_SIZE);
#ifdef DEBUG
	fprintf(stderr, "succesfully sent message, dest->num_msg: %d dest->write_pos: %d\n", dest->num_msg, dest->write_pos);	
#endif
	return 1;
}

int read_msg(struct link *link){
	const struct msg_hdr	*hdr;
	unsigned char			char_size[sizeof(unsigned int)];
	int						i, j;
	unsigned int			read_size, tmp_size, over_size;
#ifdef ASSERT
	assert(link->read_pos < BUF_SIZE);
#endif

	if(!(int)link->num_msg){
		return 0;
	}
#ifdef	MULTI
	pthread_mutex_lock(&link->rcv_mutex);
#endif	//MULTI
	hdr         = (struct msg_hdr*)&link->buf[(link)->read_pos];
	if((link->read_pos+HDR_SIZE)<BUF_SIZE){
		read_size	= HDR_SIZE + hdr->message_size;
	}
	else{
#ifdef MEM_DEBUG
		fprintf(stderr, "msg size located around end of buffer\n");
		fprintf(stderr, "link->read_pos = %d\n", link->read_pos);
#endif
		for(i=0; i+link->read_pos+12<BUF_SIZE; i++){
#ifdef MEM_DEBUG
			fprintf(stderr, "i+link->read_pos+12 = %d\n", i+link->read_pos+12);
#endif
#ifdef ASSERT
			assert(i+link->read_pos+12 >= BUF_SIZE-4 && i+link->read_pos+12<BUF_SIZE);
#endif
			char_size[i] = link->buf[link->read_pos+12+i]; 
//			memcpy(&char_size[i], &link->buf[link->read_pos+12+i], 1);
//			fprintf(stderr, "link->buf[%d] = %u, char_size[%d] = %u\n", link->read_pos+12+i, link->buf[link->read_pos+12+i], i, char_size[i]);
		}
		j = i;
		for(;i<sizeof(unsigned int); i++){
#ifdef MEM_DEBUG
			fprintf(stderr, "i = %d, j = %d\n", i, j);
			fprintf(stderr, "i-j+link->read_pos+12-BUF_SIZE = %d\n", i-j+link->read_pos+12-BUF_SIZE);
#endif
#ifdef ASSERT
			assert(i-j+link->read_pos+12-BUF_SIZE>=0 && i-j+link->read_pos+12-BUF_SIZE < 16);
#endif
			char_size[i] = link->buf[i-j+link->read_pos+12-BUF_SIZE];
//			memcpy(&char_size[i], &link->buf[(i-j)+link->read_pos+12-BUF_SIZE], 1);
//			fprintf(stderr, "link->buf[%d] = %u, char_size[%d] = %u\n", i-j, link->buf[i-j], i, char_size[i]);
		}
		memcpy(&tmp_size, &char_size[0], sizeof(unsigned int));
		read_size	= HDR_SIZE + tmp_size;
	}
	if(read_size >BUF_SIZE){
#ifdef MEM_DEBUG
		fprintf(stderr, "read_msg: over buf size\n");
#endif
		memset(link->buf, 0, sizeof(link->buf));
		link->num_msg = 0;
		return -1;		
	}
#ifdef MEM_DEBUG
	fprintf(stderr, "read_pos = %d, read_size = %d, tmp_size = %d\n", link->read_pos, read_size, tmp_size);
#endif
//assumes that the BUF_SIZE is large enough for a message
	if((link->read_pos+read_size)<BUF_SIZE){
		memcpy(&link->process_buf, hdr, read_size);
		link->read_pos += read_size;
	}
	else{
#ifdef MEM_DEBUG
		fprintf(stderr, "read_msg rounding end of buffre\n");
#endif
		over_size = (read_size+link->read_pos) - BUF_SIZE;
		memcpy(link->process_buf, hdr, read_size-over_size);
		memcpy(&link->process_buf[read_size-over_size], link->buf, over_size);
		link->read_pos = over_size;
	}
	link->num_msg -= 1;
#ifdef MEM_DEBUG
	fprintf(stderr, "message type: %s\n", link->process_buf);
#endif
#ifdef ASSERT
	assert(read_size < BUF_SIZE);
#endif
#ifdef	MULTI
	pthread_mutex_unlock(&link->rcv_mutex);
#endif	//MULTI
#ifdef DEBUG
	fprintf(stderr, "reading msg, size: %d type: %s\n", read_size, link->process_buf);
#endif
	return 1;
}
#endif
