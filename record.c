#ifndef RECORD_C
#define RECORD_C

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
#include"thread.h"
#include"record.h"

void print_block_record(){
//*record
	struct block_record *tmp, *tmp2;
	struct record_printer *printer, *save, *controll, *next;
	printer=NULL;
	next=NULL;
	printer = malloc(sizeof(struct record_printer));
	printer->same=NULL;
	for(printer->record=(struct block_record *)record;printer!=NULL;printer=next){
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
				free(tmp);
			}
		}
		//free records

		fprintf(stdout, "\n");
		//free current height records
		for(printer=save; printer!=NULL; printer=controll){
			controll=printer->same;
			free(printer);
		}			
	}
}

void print_node_s_blocks(struct miner *miner){
	struct blocks *blocks;
	if(miner->blocks!=NULL){
		fprintf(stderr, "height = %d\n", ((miner->blocks)->block)->height);
	}else{fprintf(stderr, "height = 0\n");}
	if(miner->blocks!=NULL){
		for(blocks=miner->blocks; blocks->prev!=NULL; blocks=blocks->prev){}
		fprintf(stderr, "in main chain: ");
		for(; blocks!=NULL; blocks=blocks->next){
			fprintf(stderr, "h= %d i= %d, ", (blocks->block)->height, (blocks->block)->miner_id);
		}
		fprintf(stderr,"\n");
	}
}
void add_block_record(struct miner *me, struct block *new, struct blocks *blocks){
	struct block_record *new_rec, *tmp;
	struct blocks *mine;
	unsigned int height;
#ifdef	MULTI
	pthread_mutex_lock(&block_mutex);
#endif	//MULTI
	height = new->height;	
	for(mine = blocks; mine->prev!=NULL; mine = mine->prev){}

	new_rec = malloc(sizeof(struct block_record));
	if(new_rec == NULL){
		fprintf(stderr, "couldn't malloc\n");
		exit(-1);
	}
	memset(new_rec, 0, sizeof(struct block_record));
	new_rec->next		= NULL;
	new_rec->same		= NULL;
	new_rec->mined_time = sim_time;
	new_rec->height		= new->height;
	new_rec->miner_id	= new->miner_id;
	new_rec->hash_rate	= me->hash_rate;
	new_rec->num_nodes	= 1;
//	memcpy(new_rec->hash, new->hash, SHA256_DIGEST_LENGTH);

	if(record==NULL&&height==1){
		record = new_rec;
	}
	else{
		for(tmp=record; ; tmp=tmp->next){
//			if(tmp==NULL)
//				return;
			if(tmp->height==height){
				for(;tmp->same!=NULL; tmp=tmp->same){}
				tmp->same = new_rec;
#ifdef	MULTI
				pthread_mutex_unlock(&block_mutex);
#endif	//MULTI
				return;
			}
			for(;;tmp=tmp->same){
				if(tmp==NULL){
					print_block_record();
					struct blocks *blocks;
					for(blocks=me->blocks; blocks->prev!=NULL; blocks=blocks->prev){}
					fprintf(stderr, "in main chain: ");
					for(; blocks!=NULL; blocks=blocks->next){
						fprintf(stderr, "%d, ", (blocks->block)->height);
					}
						fprintf(stderr,"\n");
				}
				if(tmp->miner_id==(mine->block)->miner_id)
					break;
			}
			if(tmp->height==height-1 && tmp->miner_id==(mine->block)->miner_id){
				if(tmp->next==NULL)
					tmp->next = new_rec;
				else{
					for(tmp=tmp->next; tmp->same!=NULL; tmp=tmp->same){}
					tmp->same = new_rec;
				}
#ifdef	MULTI
				pthread_mutex_unlock(&block_mutex);
#endif	//MULTI
				return;
			}
			mine=mine->next;
		}
	}
}

void join_record(struct block *new, struct blocks *blocks){
	struct block_record *tmp;
	struct blocks *mine;
	unsigned int height, id;
	for(mine=blocks; mine->prev!=NULL; mine=mine->prev){}
	height		= new->height;
	id			= new->miner_id;

	for(tmp=record; ; tmp=tmp->next){
		if(tmp->height==height){
			for(;;tmp=tmp->same){
				if(tmp->miner_id==id){
#ifdef	MULTI
//					pthread_mutex_lock(&block_mutex);
#endif	//MULTI

					tmp->num_nodes++;
#ifdef	MULTI
//					pthread_mutex_unlock(&block_mutex);
#endif	//MULTI
					return;
				}
			}
		}
		for(;;tmp=tmp->same){
#ifdef DEBUG
			if(tmp==NULL){
				print_block_record();
			}
#endif
			if(tmp->miner_id==(mine->block)->miner_id)
				break;
		}
		if(tmp->height==height && tmp->miner_id==id){
#ifdef	MULTI
//			pthread_mutex_lock(&block_mutex);
#endif	//MULTI

			tmp->num_nodes++;
#ifdef	MULTI
//			pthread_mutex_unlock(&block_mutex);
#endif	//MULTI
			return;
		}
		mine=mine->next;
	}
}

void print_link_record(){
	struct node_record	*n_rec, *save, *same;
	struct link_record	*l_rec, *l_next;
#ifdef PRINT_ONLY_TOTAL_AB_LINK
	int the_node, total_ab=0;
#endif
	if(n_link!=NULL){
		for(n_rec = n_link; n_rec->prev!=NULL; n_rec=n_rec->prev){printf("\n");}
		for(; n_rec!=NULL; n_rec=save){
			save=n_rec->next;
			for(; n_rec!=NULL; n_rec=same){
				same=n_rec->same;
#ifdef PRINT_ONLY_TOTAL_AB_LINK
				the_node = n_rec->my_group;
#endif //PRINT_ONLY_TOTAL_AB_LINK
#ifndef PRINT_ONLY_TOTAL_AB_LINK
				fprintf(stdout, "i= %d g= %d ", n_rec->my_id, n_rec->my_group);
#endif //PRINT_ONLY_TOTAL_AB_LINK
				l_rec=n_rec->record;
				free(n_rec);
				for(; l_rec!=NULL; l_rec=l_next){
#ifndef PRINT_ONLY_TOTAL_AB_LINK
					fprintf(stdout, "i= %d g= %d ", l_rec->dest_id, l_rec->dest_group);
#endif //PRINT_ONLY_TOTAL_AB_LINK
#ifdef PRINT_ONLY_TOTAL_AB_LINK
					if((l_rec->dest_id>=SEED_NUM && l_rec->dest_id<SEED_NUM+BAD_NODES)||(n_rec->my_id>=SEED_NUM&&n_rec->my_id<SEED_NUM+BAD_NODES)){}
					else if(the_node==-1)
						total_ab++;
					else if(the_node!=l_rec->dest_group)
						total_ab++;
#endif //PRINT_ONLY_TOTAL_AB_LINK
					l_next=l_rec->next;
					free(l_rec);
				}
#ifndef PRINT_ONLY_TOTAL_AB_LINK
				fprintf(stdout, "\n");
#endif //PRINT_ONLY_TOTAL_AB_LINK
			}
#ifdef PRINT_ONLY_TOTAL_AB_LINK
			fprintf(stdout, "sim_time= %d total_ab= %d", sim_time, total_ab/2);
#endif //PRINT_ONLY_TOTAL_AB_LINK
			fprintf(stdout, "\n");
		}
	}
	n_link=NULL;
}
void add_link_record(/*struct threads *thread*/struct miner *miner){
	struct node_record	*n_rec, *tmp;
	struct link_record	*l_rec;
	struct links		*links;
	struct miner		*me;
	int					group;	
	struct links *tmp_bad;
	me = miner;//thread->miner;

	n_rec = malloc(sizeof(struct node_record));
	if(n_rec==NULL){
		perror("malloc");
		exit(-1);
	}
	n_rec->record	= NULL;
	n_rec->same		= NULL;
	n_rec->next		= NULL;
	n_rec->prev		= NULL;
	n_rec->my_id	= me->miner_id;
	group = -1;
	if(bad_links!=NULL){
		for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
		for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
			if(tmp_bad->miner_id==me->miner_id)
				group = tmp_bad->group;
		}
	}
	n_rec->my_group	= group;
	if(me->outbound!=NULL){
		for(links=me->outbound; links->next!=NULL; links=links->next){}
		for(; links!=NULL; links=links->prev){
			if(n_rec->record==NULL){
				n_rec->record = malloc(sizeof(struct link_record));
				if(n_rec->record==NULL){
					perror("malloc");
					exit(-1);
				}
				l_rec = n_rec->record;
			}
			else{
				l_rec->next = malloc(sizeof(struct link_record));
				if(l_rec->next==NULL){
					perror("malloc");
					exit(-1);
				}
				l_rec = l_rec->next;
			}
			group = -1;
			if(bad_links!=NULL){
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==links->miner_id)
						group = tmp_bad->group;
				}
			}
			l_rec->next			= NULL;
			l_rec->dest_group	= group;
			l_rec->dest_id		= links->miner_id;
		}
	}
	if(me->inbound!=NULL){
		for(links=me->inbound; links->next!=NULL; links=links->next){}
		for(; links!=NULL; links=links->prev){
			if(n_rec->record==NULL){
				n_rec->record = malloc(sizeof(struct link_record));
				if(n_rec->record==NULL){
					perror("malloc");
					exit(-1);
				}
				l_rec = n_rec->record;
			}
			else{
				l_rec->next = malloc(sizeof(struct link_record));
				if(l_rec->next==NULL){
					perror("malloc");
					exit(-1);
				}
				l_rec = l_rec->next;
			}
			group = -1;
			if(bad_links!=NULL){
				for(tmp_bad=bad_links; tmp_bad->next!=NULL; tmp_bad=tmp_bad->next){}
				for(; tmp_bad!=NULL; tmp_bad=tmp_bad->prev){
					if(tmp_bad->miner_id==links->miner_id)
						group = tmp_bad->group;
				}
			}
			l_rec->next			= NULL;
			l_rec->dest_group	= group;
			l_rec->dest_id		= links->miner_id;
		}
	}
	if(n_link == NULL)
		n_link = n_rec;
/*
	else if(thread->next==NULL){
		n_link->next 	= n_rec;
		tmp				= n_link;
		n_link			= n_link->next;
		n_link->prev	= tmp;
	}
*/
	else{
		for(tmp = n_link; tmp->same!=NULL; tmp=tmp->same){}
		tmp->same = n_rec;
	}
}

void add_link_records(struct threads *threads){
	struct threads		*tmp;
	for(tmp=threads; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		add_link_record(tmp->miner);
	}
	int i;
	for(i=0; i<SEED_NUM; i++){
		add_link_record(seeds[i]);
	}
}

#endif
