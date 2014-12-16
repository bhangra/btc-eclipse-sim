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

#include"connection.h"
#include"proto-node.h"
#include"thread.h"
#include"global.c"

#define HDR_SIZE	16
void hexDump (char *desc, void *addr, int len);

void free_link(struct links *will_remove, struct miner *miner){
	struct links *after, *before;
	if(will_remove==NULL){
		return;
	}
	after	= will_remove->next;
	before	= will_remove->prev;
	if(will_remove!=NULL){
		if(will_remove->link!=NULL)
			free(will_remove->link);
		will_remove->link = NULL;
		free(will_remove);
		will_remove = NULL;
	}
	miner->neighbor--;
	if(after!=NULL&&before!=NULL){
		after->prev	= before;
		before->next= after;
	}
	else if(after!=NULL){
		after->prev	= NULL;
	}
	else if(before!=NULL){
		before->next= NULL;
	}
//	if(miner->links==will_remove){
		if(after!=NULL)
			miner->links=after;
		else if(before!=NULL)
			miner->links=before;
		else
			miner->links=NULL;
//	}
}

void free_links(struct threads *will_kill){
	unsigned int kill_id;
	struct threads  *tmp;
	struct miner    *miner;
	struct links    *links, *prev;
	
	kill_id = (will_kill->miner)->miner_id;
	
	for(tmp = will_kill; tmp->next!=NULL;tmp=tmp->next){}
	for(miner=tmp->miner; tmp!=NULL; tmp=tmp->prev){
		miner=tmp->miner;
		if(miner->links==NULL){
			continue;
		}
		for(links=miner->links; links->next!=NULL; links=links->next){}
		for(; links!=NULL; links=prev){
			prev = links->prev;
			if(links->miner_id==kill_id){
				free_link(links, miner);
			}
		}
	}
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
		if(miner_id==tmp->miner_id){ //already connected 
#ifdef DEBUG
			fprintf(stderr, "already connected\n");
#endif
			return links;
		}
		if(tmp->next == NULL){
			break;
		}
	}
//	fprintf(stderr, "will malloc\n"); //debug

	new			= malloc(sizeof(struct links));
	if(new==NULL){
		perror("malloc");
		exit(-1);
	}
	memset(new, 0, sizeof(struct links));
	new->link	= malloc(sizeof(struct link));
	if(new->link==NULL){
		perror("malloc");
		exit(-1);
	}
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
#ifdef DEBUG
	fprintf(stderr, "sending msg_size: %d %s\n", msg_size, message); //debug
#endif

	if(dest->write_pos > BUF_SIZE){
		dest->read_pos = 0;
		dest->write_pos= 0;
		memset(dest->buf, 0, sizeof(dest->buf));
	}
	pos			= dest->write_pos;
	tmp			= pos + msg_size;
	if(pos+msg_size >=BUF_SIZE)
		tmp			= pos+msg_size-BUF_SIZE;
//	fprintf(stderr, "dest->write_pos = %d, dest->read_pos = %d\n", pos, dest->read_pos);
	if(pos < dest->read_pos && (pos+msg_size >= dest->read_pos) && pos+msg_size <BUF_SIZE){
//		fprintf(stderr, "catched up to read_pos\n");
		return 0;
	}
	if((pos < dest->read_pos) && (tmp <= dest->read_pos) && pos+msg_size>BUF_SIZE){
//		fprintf(stderr, "rounded around buffer, catching up to read_pos. tmp = %d\n", tmp);
		return 0;
	}
	if(pos > dest->read_pos && tmp >= dest->read_pos && pos+msg_size > BUF_SIZE){
		return 0;
	}
//	fprintf(stderr, "dest->write_pos = %d\n", dest->write_pos);
	pthread_mutex_lock((pthread_mutex_t *)&dest->rcv_mutex);
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
	pthread_mutex_unlock((pthread_mutex_t *)&dest->rcv_mutex);
	memset(message, 0, BUF_SIZE);
	return 1;
}

int read_msg(struct link *link){
	const struct msg_hdr	*hdr;
	unsigned char			char_size[sizeof(unsigned int)];
	int						i, j;
	unsigned int			read_size, tmp_size, over_size;
	if(!(int)link->num_msg){
		return 0;
	}
	pthread_mutex_lock(&link->rcv_mutex);
	hdr         = (struct msg_hdr*)&link->buf[(link)->read_pos];
	if((link->read_pos+HDR_SIZE)<BUF_SIZE){
		read_size	= HDR_SIZE + hdr->message_size;
	}
	else{
		i=0;
		for(i=0; i+link->read_pos+12<BUF_SIZE; i++){
//			char_size[i] = link->buf[link->read_pos+12+i]; 
			memcpy(&char_size[i], &link->buf[link->read_pos+12+i], 1);
//			fprintf(stderr, "link->buf[%d] = %u, char_size[%d] = %u\n", link->read_pos+12+i, link->buf[link->read_pos+12+i], i, char_size[i]);
		}
		j = i;
		for(;i<sizeof(unsigned int); i++){
//			char_size[i] = link->buf[i-j];
			memcpy(&char_size[i], &link->buf[(i-j)+link->read_pos+12-BUF_SIZE], 1);
//			fprintf(stderr, "link->buf[%d] = %u, char_size[%d] = %u\n", i-j, link->buf[i-j], i, char_size[i]);
		}
		memcpy(&tmp_size, &char_size[0], sizeof(unsigned int));
		read_size	= HDR_SIZE + tmp_size;
	}
	if(read_size >BUF_SIZE){
#ifdef DEBUG
		fprintf(stderr, "read_msg: over buf size\n");
#endif
		memset(link->buf, 0, sizeof(link->buf));
		link->num_msg = 0;
		return -1;		
	}
//#ifdef DEBUG
//	fprintf(stderr, "read_pos = %d, read_size = %d\n", link->read_pos, read_size);
//#endif
//assumes that the BUF_SIZE is large enough for a message
	if((link->read_pos+read_size)<BUF_SIZE){
		memcpy(&link->process_buf, hdr, read_size);
		link->read_pos += read_size;
	}
	else{
		over_size = (read_size+link->read_pos) - BUF_SIZE;
		memcpy(link->process_buf, hdr, read_size-over_size);
		memcpy(&link->process_buf[read_size-over_size], link->buf, over_size);
		link->read_pos = over_size;
	}
	link->num_msg -= 1;
	pthread_mutex_unlock(&link->rcv_mutex);
//	fprintf(stderr, "reading msg, size: %d type: %s\n", read_size, link->process_buf);
	return 1;
}
#endif
