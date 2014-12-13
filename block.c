#ifndef BLOCK_C
#define BLOCK_C

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"global.c"
#include"block.h"
#include"connection.h"
#include"proto-node.h"

void get_blocks(struct link *dest, struct blocks *main_chain, struct blocks *new_chain);
void request_block(unsigned int wanted_height, struct link *dest);
void propagate_block(struct block *block, struct miner *me);

void print_record(){
//*record
	struct block_record *tmp, *tmp2;
	struct record_printer *printer, *save, *controll, *next;
	printer=NULL;
	next=NULL;
	printer = malloc(sizeof(struct record_printer));
	printer->same=NULL;
	for(printer->record=record;printer!=NULL;printer=next){
		next=NULL;
		//malloc for next height records
		for(save=printer;printer!=NULL; printer=printer->same){
			for(tmp=printer->record;tmp!=NULL; tmp=tmp->same){
				if(tmp->next!=NULL){
					if(next==NULL){
						next=malloc(sizeof(struct record_printer));
						next->same=NULL;
						next->record=tmp->next;
					}else if(next->same==NULL){
						next->same=(malloc(sizeof(struct record_printer)));
						controll = next->same;
						controll->record=tmp->next;
						controll->same = NULL;
					}
					else{
						controll->same=malloc(sizeof(struct record_printer));
						controll=controll->same;
						controll->record=tmp->next;
						controll->same=NULL;
					}
				}
			}
		}
		//print
		for(printer=save;printer!=NULL;printer=printer->same){
			for(tmp=printer->record; tmp!=NULL; tmp=tmp2){
				fprintf(stdout, "t= %d h= %d i= %d r= %f n= %d ",tmp->mined_time, tmp->height, tmp->miner_id, tmp->hash_rate, tmp->num_nodes); 
				tmp2 = tmp->same;
//				free(tmp);
			}
		}
		fprintf(stdout, "\n");
		//free current height records
		for(printer=save; printer!=NULL; printer=controll){
			controll=printer->same;
			free(printer);
		}			
	}
}


void add_record(struct miner *me, struct block *new, struct blocks *blocks){
	struct block_record *new_rec, *tmp;
	struct blocks *mine;
	unsigned int height;

	height = new->height;	
	for(mine = blocks; mine->prev!=NULL; mine = mine->prev){}

	new_rec = malloc(sizeof(struct block_record));
	memset(new_rec, 0, sizeof(struct block_record));
	new_rec->next		= NULL;
	new_rec->same		= NULL;
	new_rec->mined_time = sim_time;
	new_rec->height		= new->height;
	new_rec->miner_id	= me->miner_id;
	new_rec->hash_rate	= me->hash_rate;
	new_rec->num_nodes	= 1;
//	memcpy(new_rec->hash, new->hash, SHA256_DIGEST_LENGTH);

	if(record==NULL&&height==1){
		record = new_rec;
	}
	else{
		for(tmp=record; ; tmp=tmp->next){
			if(tmp==NULL)
				return;
			if(tmp->height==height){
				for(;tmp->same!=NULL; tmp=tmp->same){}
				tmp->same = new_rec;
				return;
			}
			for(;;tmp=tmp->same){
				if(tmp->miner_id==(mine->block)->miner_id)
					break;
				if(tmp->same==NULL)
					return;
			}
			if(tmp->next==NULL && tmp->height==height-1 && tmp->miner_id==(mine->block)->miner_id){
				tmp->next = new_rec;
				return;
			}
			mine=mine->next;
		}
	}
}

void join_record(struct block *new, struct blocks *blocks){
	struct block_record *tmp;
	struct blocks *mine;
	unsigned int height, miner_id;
	for(mine=blocks; mine->prev!=NULL; mine=mine->prev){}
	height		= new->height;
	miner_id	= new->miner_id;

	for(tmp=record; ; tmp=tmp->next){
//		if(tmp==NULL)
//			return;
		if(tmp->height==height){
			for(;; tmp=tmp->same){
				if(tmp->miner_id==miner_id){
					tmp->num_nodes++;
					return;
				}
			}
		}
		for(;;tmp=tmp->same){
			if(tmp->miner_id==(mine->block)->miner_id)
				break;
//			if(tmp==NULL)
//      		return;
		}
/*		if(tmp->next==NULL && tmp->height==height-1){
			tmp->num_nodes++;
			return;
		}
*/		mine=mine->next;
	}
}

struct blocks *add_block(struct block *block, struct blocks *chain_head){
	struct blocks *tmp, *tmp2;
	tmp = malloc(sizeof(struct blocks));
	memset(tmp, 0, sizeof(struct blocks));
	tmp->block          = block;
	if(chain_head!=NULL){
		for(tmp2=chain_head; tmp2->next!=NULL; tmp2=tmp2->next){}
		tmp2->next			= tmp;
		tmp->prev           = tmp2;
		tmp->next           = NULL;
	}else{
		tmp->prev           = NULL;
		tmp->next           = NULL;
	}
	return tmp;
}

struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct miner *me, struct link *from){
	struct block	*accept, *head;
	struct blocks	*tmp, *tmp2, *tmp3;
//	fprintf(stderr, "will process block: chain_head= %p new_chain=%p\n", chain_head, me->new_chain);
	if(chain_head==NULL&&block->height==1){
		fprintf(stderr, "genesis block received\n"); //debug
		accept = malloc(sizeof(struct block));
		memcpy(accept, block, sizeof(struct block));
		propagate_block(accept, me);
//		join_record(accept, me->blocks);
		me->blocks=add_block(accept, NULL);
		join_record(accept, me->blocks);
		return me->blocks;
	}
	if(chain_head!=NULL){
		fprintf(stderr, "will check if it's next block\n");
		for(tmp=chain_head; tmp->next!=NULL; tmp=tmp->next){}
		head = chain_head->block;
		if(block->height == head->height+1 && !memcmp(block->hash, SHA256((char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
			fprintf(stderr, "next block received\n"); //debug
			accept = malloc(sizeof(struct block));
			memcpy(accept, block, sizeof(struct block));
			propagate_block(accept, me);
//			join_record(accept, me->blocks);
			tmp = add_block(accept, chain_head);
			join_record(accept, me->blocks);
			return tmp;
		}
	}
	if(me->new_chain!=NULL){
		fprintf(stderr, "will check in the new_chain\n");
		for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){
			if(tmp->next==NULL){break;}
		}
		head=tmp->block;
		fprintf(stderr, "will check, if it's next one to new_chain\n");
		if(block->height == head->height + 1 && !memcmp(block->hash, SHA256((char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
			fprintf(stderr, "new block added to new_chain's head\n"); //debug
			accept = malloc(sizeof(struct block));
			memcpy(accept, block, sizeof(struct block));
			me->new_chain = add_block(accept, tmp);
			for(; tmp->prev!=NULL; tmp=tmp->prev){}
			head = tmp->block;
//			request_block(head->height-1, from);
//			request_block(head->height-2, from);
//			request_block(head->height-3, from);
//			request_block(head->height-4, from);
//			request_block(head->height-5, from);
			get_blocks(from, me->blocks, me->new_chain);
			return chain_head;
		}
		fprintf(stderr, "will check, if it's previous one to new_chain\n");
		for(;;tmp=tmp->prev){
			fprintf(stderr, "tmp = %p, tmp->block = %p\n", tmp, tmp->block);
			if(tmp==NULL){
				return chain_head;
			}
			head = tmp->block;
			if(head->height == block->height+1 && !memcmp(head->hash, SHA256((char *)block, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
				fprintf(stderr,"new block added to tail of new_chain\n"); //debug
				accept = malloc(sizeof(struct block));
				memcpy(accept, block, sizeof(struct block));
				tmp2 = malloc(sizeof(struct blocks));
				tmp2->block	= accept;
				tmp2->prev	= NULL;
				tmp2->next	= tmp;
				tmp->prev	= tmp2;
				tmp3		= tmp;
				if(accept->height == 1){
					for(tmp=me->blocks; tmp->next=NULL; tmp=tmp->next){
						if(tmp==NULL||tmp->next==NULL){break;}
					}
					for(; tmp!=NULL; tmp=tmp2){
						tmp2 = tmp->prev;
						free(tmp->block);
						if((struct links*)tmp!=(struct links*)&me->blocks)
							free(tmp);
					}			
					for(tmp=tmp3; tmp->next!=NULL; tmp=tmp->next){}
					me->new_chain = NULL;
					propagate_block(tmp->block, me);
					join_record(tmp->block, me->blocks);
					return tmp;
				}
				else{
				for(tmp=chain_head; tmp!=NULL; tmp=tmp->prev){
					head=tmp->block;
					if(accept->height==head->height+1 && !memcmp(accept->hash, SHA256((char *)head, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
						tmp3		= tmp->next;
						tmp->next	= tmp2;
						tmp2->prev	= tmp;
						for(tmp=tmp3; tmp!=NULL; tmp=tmp3){
							tmp3	= tmp->next;
							free(tmp->block);
							free(tmp);						
						}
						for(tmp=tmp2; tmp->next!=NULL;  tmp=tmp->next){}
						me->new_chain=NULL;
						propagate_block(tmp->block, me);
						join_record(tmp->block, me->blocks);
						return tmp;
					}
				}
				}
//				request_block(block->height-1, from);
//				request_block(block->height-2, from);
//				request_block(block->height-3, from);
//				request_block(block->height-4, from);
//				request_block(block->height-5, from);
				get_blocks(from, me->blocks, me->new_chain);
				return chain_head;
			}
			if(tmp->prev==NULL){break;}
		}
		fprintf(stderr, "check in new_chain finished\n");
		if(block->height <= head->height){
			fprintf(stderr, "in new_chain: equal or lower block received\n"); //debug
			return chain_head;
		}
	}
//	if(block->height>1&&chain_head==NULL){
//		request_block(block->height-1, from);
//	}
	fprintf(stderr, "will check if it's lower or equal\n");

	if(block->height > 1 && chain_head==NULL){
		if(me->new_chain!=NULL){
			fprintf(stderr, "free new_chain\n");
			for(tmp=me->new_chain; tmp->next=NULL; tmp=tmp->next){
				if(tmp==NULL||tmp->next==NULL){break;}
			}
			for(; tmp!=NULL; tmp=tmp2){
				tmp2 = tmp->prev;
				free(tmp->block);
				if((struct links*)tmp!=(struct links*)&me->new_chain)
					free(tmp);
			}
		}
		me->new_chain = malloc(sizeof(struct blocks));
		memset(me->new_chain, 0, sizeof(struct blocks));
		accept = malloc(sizeof(struct block));
		memcpy(accept, block, sizeof(struct block));
		(me->new_chain)->block	= accept;
		(me->new_chain)->next	= NULL;
		(me->new_chain)->prev	= NULL;
		fprintf(stderr, "will request for it's prev blocks\n");
//		request_block(block->height-1, from);
//		request_block(block->height-2, from);
//		request_block(block->height-3, from);
//		request_block(block->height-4, from);
//		request_block(block->height-5, from);
		get_blocks(from, me->blocks, me->new_chain);

		return chain_head;
    }
	fprintf(stderr, "check if higher than my chain height\n");
	head = chain_head->block;
	if(block->height > head->height+1){
		fprintf(stderr, "need previous block\n"); //debug
		accept = malloc(sizeof(struct block));
		memcpy(accept, block, sizeof(struct block));
		if(me->new_chain!=NULL){
			fprintf(stderr, "free new_chain\n");
			for(tmp=me->new_chain; tmp->next!=NULL; tmp=tmp->next){
				if(tmp==NULL||tmp->next==NULL){break;}
			}
			for(; tmp!=NULL; tmp=tmp2){
				tmp2 = tmp->prev;
				free(tmp->block);
				if((struct links*)tmp!=(struct links*)&me->new_chain)
					free(tmp);
			}
		}
		me->new_chain = malloc(sizeof(struct blocks));
		memset(me->new_chain, 0, sizeof(struct blocks));
		(me->new_chain)->block = accept;
		(me->new_chain)->next = NULL;
		(me->new_chain)->prev = NULL;
		fprintf(stderr, "will request for it's prev block\n");
//		request_block(block->height-1, from);
//		request_block(block->height-2, from);
//		request_block(block->height-3, from);
//		request_block(block->height-4, from);
//		request_block(block->height-5, from);
		get_blocks(from, me->blocks, me->new_chain);
		return chain_head;
	}
	fprintf(stderr, "block received not added\n");
	return chain_head;
}

/*
struct blocks *connect_block(struct block *block, struct blocks *new_chain){
	struct blocks *tmp;
	tmp = malloc(sizeof(struct blocks));
	memset(tmp, 0, sizeof(struct blocks));
	tmp->block          = block;    
	if(new_chain!=NULL){
		new_chain->prev     = tmp;
		tmp->next           = new_chain;
		tmp->next           = NULL;
	}else{
		tmp->prev           = NULL;
		tmp->next           = NULL;
	}
	return tmp;
}

struct blocks *search_prev(struct blocks *chain_head, struct blocks *new_chain){
	struct block *new, *search;
	struct blocks *tmp;
	new = new_chain->block;
	if(new->height==1){
		tmp = new_chain;
		return tmp;
	}
	for(tmp=chain_head; tmp->prev!=NULL; tmp=tmp->prev){
		search = tmp->block;
		if(!memcmp(new->prev, SHA256((char *)search, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){  
			tmp->next       = new_chain;
			new_chain->prev = tmp;
			for(tmp=new_chain; tmp->next!=NULL; tmp=tmp->next){}
			return tmp;
		}
	}
}

int verify_block(struct block *new_block, struct blocks *chain_head, struct blocks *new_chain){

	struct block *tmp;
	struct blocks *blocks;
	if(new_chain!=NULL){
		for(blocks = new_chain; blocks->prev!=NULL; blocks=blocks->prev){;}
		tmp = blocks->block;
		if(!memcmp(tmp->prev, SHA256((char *)tmp, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
			return 2;
		}
	}
	else if(chain_head==NULL&&new_block->height ==1)
		return 1;

	tmp = chain_head->block;
	if(tmp->height >= new_block->height){
		fprintf(stderr, "new block's height equal or lower than mine\n");//debug
		return -1;
	}
	else if(memcmp(new_block->prev, SHA256((char *)tmp, sizeof(struct block), 0), SHA256_DIGEST_LENGTH)){
		fprintf(stderr, "don't have new block's previous block\n"); //debug
		return 0;
	}
	return 1;

}

struct blocks *process_new_blocks(struct block *block, struct blocks *chain_head, struct miner *me, struct link *from){
    int rerun;
    int validity;
    struct block *accept;
    struct blocks *tmp;
    fprintf(stderr, "process_new_blocks()\n"); //debug
    tmp = chain_head;
    for(rerun = 1; rerun==1;){
        rerun = 0;
        for(;;){
            validity = verify_block(block, tmp, me->new_chain);
            fprintf(stderr, "checked validity\n");
            if(validity==2){
                fprintf(stderr, "added to new_chain\n");
                me->new_chain   = connect_block(block, me->new_chain);
                me->blocks      = search_prev(me->blocks, me->new_chain);
                return me->blocks;
            }
            if(validity==1){
                accept = malloc(sizeof(block));
                memcpy(accept, block, sizeof(block));
                fprintf(stderr, "accepted block with height: %d\n", accept->height); // debug
                return add_block(accept, chain_head);
            }
            else if(validity==0){
                me->new_chain = connect_block(block, me->new_chain);
                request_block((block->height-1), from);
                if(tmp->prev==NULL)
                    return chain_head;
                tmp = tmp->prev;
            }
            else if(validity==-1){
                return chain_head;
            }
        }
    }
    return chain_head;
}
*/
#endif
