#ifndef ADDRMAN_C
#define ADDRMAN_C
#include"global.c"
#include"connection.h"
#include"addrman.h"
#include"thread.h"
#define max(A,B) (((A)>(B))?(A):(B)) 


int shrink_new(struct addrman *addrman, int n_u_bucket, unsigned int nid);

struct caddrinfo *map_info(struct addrman *addrman, unsigned int nid){
	struct caddrinfo *tmp;
	if(addrman->caddrinfo==NULL)
		return NULL;
	for(tmp = addrman->caddrinfo; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		if(tmp->nid == nid)
			return tmp;
	}
	return NULL;
}
unsigned int map_addr(struct addrman *addrman, struct links *links){
	struct caddrinfo *tmp;
	if(addrman->caddrinfo==NULL)
		return 0;
	for(tmp=addrman->caddrinfo; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		if(tmp->new_comer==links->new_comer && tmp->miner_id==links->miner_id)
			return tmp->nid;
	}
	return 0;
}
void hexDump (char *desc, void *addr, int len);

void add(unsigned char *vaddr, struct caddrinfo *ai){
#ifdef ADDR_DEBUG
	fprintf(stderr, "add(ai): miner_id = %d, new_comer = %p, n_time = %d, subnet = %d\n", ai->miner_id, ai->new_comer, ai->n_time, ai->subnet);
#endif
	memcpy(vaddr, &ai->miner_id, sizeof(unsigned int));
	memcpy(&vaddr[sizeof(unsigned int)], &ai->new_comer, sizeof(struct link*));;
	memcpy(&vaddr[sizeof(unsigned int)+sizeof(struct link*)], &ai->n_time, sizeof(unsigned int));
	memcpy(&vaddr[sizeof(unsigned int)+sizeof(struct link*)+sizeof(unsigned int)], &ai->subnet, sizeof(unsigned int)); 
//	hexDump("getaddr_ add", vaddr, sizeof(struct link*)+sizeof(unsigned int)*3);
}

void erase(struct addrman *addrman, int erase_pos){
//	fprintf(stderr, "erase_pos = %d, v_random_size = %d\n", erase_pos, addrman->v_random_size);
	int i;
	for(i=0; i<addrman->v_random_size-erase_pos&&i</*sizeof(addrman->v_random)+1*/1000; i++){
//		fprintf(stderr, "i= %d\n", i);
		addrman->v_random[erase_pos+i] = addrman->v_random[erase_pos+1+i];
	}

//	memcpy(&addrman->v_random[erase_pos], &addrman->v_random[erase_pos+1], addrman->v_random_size-erase_pos);

/*unsigned int		tmp[ADDRMAN_NEW_BUCKET_COUNT+ADDRMAN_TRIED_BUCKET_COUNT][ADDRMAN_BUCKET_SIZE];

//	fprintf(stderr, "erase_pos = %d, v_random_size = %d\n", erase_pos, addrman->v_random_size);
	memcpy(&tmp, &addrman->v_random[erase_pos+1], addrman->v_random_size-erase_pos);
	memcpy(&addrman->v_random[erase_pos], &tmp, addrman->v_random_size-erase_pos);
*/
}

unsigned int get_tried_bucket(struct caddrinfo *info, struct addrman *addrman){
	unsigned int	hash1 = rand()%ADDRMAN_TRIED_BUCKETS_PER_GROUP;
	unsigned int	hash2;
	unsigned char 	ss2[sizeof(unsigned int)*2];
	memcpy(&ss2, &hash1, sizeof(unsigned int));
	memcpy(&ss2[sizeof(unsigned int)], &info->subnet, sizeof(unsigned int));
	
	memcpy(&hash2, SHA256((const unsigned char*)&ss2, sizeof(ss2), 0), sizeof(hash2));
	return hash2%ADDRMAN_TRIED_BUCKET_COUNT;//will fix
}

unsigned int get_new_bucket(struct caddrinfo *info, struct addrman *addrman){
	unsigned int	hash1 = rand()%ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP;
	unsigned int	hash2;
	unsigned char	ss2[sizeof(struct link*)+4];
	memcpy(&ss2, &hash1, sizeof(unsigned int));
	memcpy(&ss2[sizeof(unsigned int)], &info->source, sizeof(struct link*));
	memcpy(&hash2, SHA256((const unsigned char*)&ss2, sizeof(ss2), 0), sizeof(hash2));
	return hash2%ADDRMAN_NEW_BUCKET_COUNT;//will fix
}

bool is_terrible(unsigned int n_now, struct caddrinfo *info){
	if(info==NULL)
		return true;
	if(info->n_last_try && info->n_last_try >= n_now-60)
		return false; //don't remove things tried in the last minute
	
	if(info->n_time > n_now + 10*60) //came in a flying DeLorean
		return true;

	if(info->n_time==0 || n_now - info->n_time > ADDRMAN_HORIZON_DAYS * 24 * 60 * 60)
		return true; //not seen in recent history

	if(info->n_last_success == 0 && info->n_attempts >= ADDRMAN_RETRIES)
		return true; //tried N times & never a success

	if(n_now - info->n_last_success > ADDRMAN_MIN_FAIL_DAYS * 24 * 60 * 60 && info->n_attempts >= ADDRMAN_MAX_FAILURES)
		return true; //N successive failures in the last week

	return false;
}

double get_chance(unsigned int n_now, struct caddrinfo *info){
	double f_chance = 1.0;
	
	int n_since_last_seen	= n_now - info->n_time;
	int n_since_last_try	= n_now - info->n_last_try;
	
	if(n_since_last_seen < 0)
		n_since_last_seen = 0;
	if(n_since_last_try < 0)
		n_since_last_try = 0;
//	fprintf(stderr, "n_since_last_seen = %d\n", n_since_last_seen);
	f_chance *= (double)(600.0 / (600.0+(double)n_since_last_seen));
//	fprintf(stderr, "(600 / (600+n_since_last_seen)) = %d\n", (600 / (600+n_since_last_seen)));
//	fprintf(stderr, "f_chance *= 1 = %f\n", f_chance); 
	//deprioritize very recent attempts away
	if(n_since_last_try < 60 * 10)
		f_chance *= 0.01;
	int n;
	//deprioritize 50% after each failed attempt
	for(n=0; n < info->n_attempts; n++)
		f_chance /= 1.5;
//	fprintf(stderr, "f_chance = %f\n", f_chance);
	return f_chance;
}
//will fix
struct caddrinfo *find(struct addrman *addrman, struct links *links, int* pnid){
	unsigned int 	 it = map_addr(addrman, links);
	struct caddrinfo *tmp=NULL;
/*	if(addrman->caddrinfo==NULL)
		return NULL;
	for(tmp=addrman->caddrinfo; tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		if(tmp->new_comer==links->new_comer&&tmp->miner_id==links->miner_id)
			break;
//			return tmp;
	}
*/
	tmp = map_info(addrman, it);
//	if(it==0)
//		return NULL;
	if(pnid)
		*pnid = it;
	return tmp;
//	return NULL;
}

struct caddrinfo *create(struct addrman *addrman, struct links *links, struct link *addr_src){
	/*int nid =*/ addrman->n_id_count++;
	int nid = addrman->n_id_count;
	if(nid==0){
		nid++;
		addrman->n_id_count++;
	}
	struct caddrinfo *new = malloc(sizeof(struct caddrinfo));
	memset(new, 0, sizeof(struct caddrinfo));
	new->next=NULL;
	new->prev=NULL;
	new->new_comer	= links->new_comer;
	new->miner_id	= links->miner_id;
	new->subnet		= links->subnet;
	new->source		= addr_src;
	new->nid		= nid;//addrman->n_id_count++;
#ifdef ADDR_DEBUG
	fprintf(stderr, "caddrinfo *create: new->new_comer: %p, new->miner_id = %d\n", new->new_comer, new->miner_id);
#endif
	int i;
	for(i=0; i<sizeof(addrman->v_random)/sizeof(unsigned int); i++){
#ifdef ADDR_DEBUG
		fprintf(stderr, "addrman->v_random[%d] == %d\n", i, addrman->v_random[i]);
#endif
		if(addrman->v_random[i]==0){
			addrman->v_random[i] = nid;
			addrman->v_random_size++;
			break;
		}			
	}
	if(i==sizeof(addrman->v_random)/sizeof(unsigned int)){
		addrman->v_random_size=i;
		for(;addrman->v_random_size==sizeof(addrman->v_random)/sizeof(unsigned int);){
			shrink_new(addrman, rand()%ADDRMAN_NEW_BUCKET_COUNT, 0-1);
		}
	}
	struct caddrinfo *tmp;
	if(addrman->caddrinfo!=NULL){
		for(tmp=addrman->caddrinfo; tmp->next!=NULL; tmp=tmp->next){}
		tmp->next=new;
		new->prev=tmp;
	}
	addrman->caddrinfo = new;
	return new;
}

void swap_random(struct addrman *addrman, unsigned int n_rnd_pos1, unsigned int n_rnd_pos2){
	if(n_rnd_pos1 == n_rnd_pos2)
		return;
//	struct caddrinfo *tmp;
	int nid1 = addrman->v_random[n_rnd_pos1];
	int nid2 = addrman->v_random[n_rnd_pos2];
	addrman->v_random[n_rnd_pos1] = nid2;
	addrman->v_random[n_rnd_pos2] = nid1;
}

int select_tried(struct addrman *addrman, unsigned int nkbucket){
	struct caddrinfo *tmp=NULL, *tmp2=NULL;
	unsigned int *v_tried = &addrman->vv_tried[nkbucket][0];
	int n_oldest	= -1;
	int n_oldest_pos= -1;
	unsigned int i;
	unsigned int v_tried_size = 0;
	for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
		if(v_tried[i]!=0)
			v_tried_size++;
	}
	
	for(i=0; i<ADDRMAN_TRIED_ENTRIES_INSPECT_ON_EVICT && i < v_tried_size; i++){ //will fix v_tried_size
/*		for(tmp=addrman; tmp->next!=NULL; tmp=tmp->next){}
		for(; tmp!=NULL; tmp=tmp->prev){
			if(tmp->nid==v_tried[i])
				break;
		}
		if(n_oldest==0)
			n_oldest_pos=i;
		if(tmp2!=NULL){
			if(tmp->n_last_success < tmp2->n_last_success){
				n_oldest_pos=i;
				tmp2=tmp;
			}
		}
*/		int npos		= (rand()%(v_tried_size - i)) + i;
		int ntemp		= v_tried[npos];
		v_tried[npos]	= v_tried[i];
		v_tried[i]		= ntemp;
		if(n_oldest!=-1){
			tmp = map_info(addrman, ntemp);
			tmp2= map_info(addrman, n_oldest);
		}
		if(n_oldest==-1){
			n_oldest		= ntemp;
			n_oldest_pos	= npos;
		}
		else if(tmp!=NULL&&tmp2!=NULL)
			if(tmp->n_last_success < tmp2->n_last_success){
				n_oldest		= ntemp;
				n_oldest_pos	= npos;
			}
	}
	return n_oldest_pos;
}
//the only function that deletes addr from addrman
int shrink_new(struct addrman *addrman, int n_u_bucket, unsigned int nid){
//	fprintf(stderr, "shrink_new\n");
	int *vnew = (int *)&addrman->vv_new[n_u_bucket][0];
	int i, j;
	struct caddrinfo *tmp=NULL, *prev, *next;
	//look for deletable items
	for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
		if(vnew[i]==nid)
				continue;
		if(vnew[i]!=0){
			tmp = map_info(addrman, vnew[i]);
			if(tmp==NULL){
				vnew[i]=0;
				return 0;
			}
			else{
				if(is_terrible(sim_time, tmp)){
					if(tmp->n_ref_count-- <= 0){
						prev = tmp->prev;
						next = tmp->next;
						if(prev!=NULL)
							prev->next = next;
						if(next!=NULL)
							next->prev = prev;
						if(addrman->caddrinfo==tmp){
							if(next!=NULL)
								addrman->caddrinfo = next;
							else
								addrman->caddrinfo = prev;
						}
						free(tmp);
						addrman->n_new--;
						
						for(j=0; j<addrman->v_random_size&&j<sizeof(addrman->v_random); j++){
							if((int)addrman->v_random[j]==vnew[i]){
//								addrman->v_random[j]=0;
								erase(addrman, j);
								break;
							}
						}
					}
				vnew[i]=0;
				return 0;
				}
			}
		}
	}
	//select 4 randomly, picking oldest and replace
/*	int n[4] = {rand()%addrman->v_random_size, rand()%addrman->v_random_size, rand()%addrman->v_random_size, rand()%addrman->v_random_size};
*/	int n[4] = {vnew[rand()%ADDRMAN_BUCKET_SIZE],vnew[rand()%ADDRMAN_BUCKET_SIZE],vnew[rand()%ADDRMAN_BUCKET_SIZE],vnew[rand()%ADDRMAN_BUCKET_SIZE]};
	for(;n[0]==nid&&n[1]==nid&&n[2]==nid&&n[3]==nid;){
		n[0] = vnew[rand()%ADDRMAN_BUCKET_SIZE];
		n[1] = vnew[rand()%ADDRMAN_BUCKET_SIZE];
		n[2] = vnew[rand()%ADDRMAN_BUCKET_SIZE];
		n[3] = vnew[rand()%ADDRMAN_BUCKET_SIZE];
	}
	int ni;
	int noldest = -1;
	struct caddrinfo *tmp1, *tmp2;
	for(ni=0; ni<4; ni++){
		if(n[ni]==nid)
			continue;
		if(noldest!=-1){
			tmp1=map_info(addrman, noldest);
			tmp2=map_info(addrman, n[ni]);
			if(tmp1==NULL)
				break;
			if(tmp2==NULL)
				break;
		}
		if(noldest==-1)
			noldest = n[ni];
		else if(tmp2->n_time < tmp1->n_time){
			noldest = n[ni];
		}
	}
	struct caddrinfo *oldest;
	oldest = map_info(addrman, noldest);
	if(oldest!=NULL)
	if(oldest->n_ref_count-- <= 0){
		prev = oldest->prev;
		next = oldest->next;
		if(prev!=NULL)
			prev->next = next;
		if(next!=NULL)
			next->prev = prev;
		if(addrman->caddrinfo==oldest){
			if(next!=NULL)
				addrman->caddrinfo	= next;
			else
				addrman->caddrinfo		= prev;
		}
		free(oldest);
		for(j=0; j<addrman->v_random_size; j++){
			if(addrman->v_random[j]==noldest){
				erase(addrman, j);
				break;
			}
		}
	}

	addrman->n_new--;
//	vnew[noldest] = 0;
	for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
		if(vnew[i] == noldest){
			vnew[i]=0;
			break;
		}
	}
	return 1;
}

void make_tried(struct addrman *addrman, struct caddrinfo *info, unsigned int nid, int n_u_bucket2){
	//remove the entry from new buckets
	unsigned int /*count,*/ /*tmp,*/ i, j/*, *vv_new[][]*/, *vv_tried;
//	count = 0;
//	vv_new=&addrman->vv_new;
	for(i=0; i<ADDRMAN_NEW_BUCKET_COUNT; i++){
		for(j=0; j<ADDRMAN_BUCKET_SIZE; j++){
			if(addrman->vv_new[i][j]==nid)				
				addrman->vv_new[i][j]=0;
		}
	}
	if(addrman->n_new!=0)
		addrman->n_new--;

	//selects which tried bucket to move into
	i = get_tried_bucket(info, addrman);
	vv_tried=&addrman->vv_tried[i][0];

	//check if there is place to add it
	int k;
	for(k=0; k<ADDRMAN_BUCKET_SIZE; k++){
		if(vv_tried[k]==0){
#ifdef ADDR_DEBUG
			fprintf(stderr, "asserted nid: %d in vv_tried[%d][%d]\n", nid, i, k);
#endif
			vv_tried[k]=nid;
			addrman->n_tried++;
			info->n_ref_count= 1;
			info->f_in_tried = true;
			return;
		}
	}

	//else remove bad item 
	int n_pos = select_tried(addrman, i);
	struct caddrinfo *tmp = map_info(addrman, addrman->vv_tried[i][n_pos]);
	if(tmp!=NULL){
		//select which new bucket the bad item belongs to
		int n_u_bucket = get_new_bucket(tmp, addrman);
		struct caddrinfo *info_old;
		for(info_old=addrman->caddrinfo; info_old->next!=NULL; info_old=info_old->next){}
		for(; info_old!=NULL; info_old=info_old->prev){
			if(info_old->nid==vv_tried[n_pos])
				break;
		}
		info_old->f_in_tried	= false;
		info_old->n_ref_count	= 1;
		
		for(k=0; k<ADDRMAN_BUCKET_SIZE; k++){
			if(addrman->vv_new[n_u_bucket][k]==0){
				addrman->vv_new[n_u_bucket][k]=info_old->nid;
			}
		}
		if(k==ADDRMAN_BUCKET_SIZE){
			addrman->vv_new[i][j] = info_old->nid;
		}
		addrman->n_new++;
	}
	else{		
		addrman->n_tried++;
	}
	vv_tried[n_pos] = nid;
	info->f_in_tried = true;
	return;
}

void addrman_good(struct addrman *addrman, struct link *new_comer, unsigned int ntime){
	struct caddrinfo *tmp;
	int nid;
	for(tmp=addrman->caddrinfo;tmp->next!=NULL; tmp=tmp->next){}
	for(; tmp!=NULL; tmp=tmp->prev){
		if(tmp->new_comer==new_comer){
			tmp->n_last_success = ntime;
			tmp->n_last_try		= ntime;
			tmp->n_time			= ntime;
			tmp->n_attempts		= 0;
			nid					= tmp->nid;
			break;
		}
	}
	if(tmp!=NULL)
		if(tmp->f_in_tried)
			return;
//	int nrnd = rand()%ADDRMAN_NEW_BUCKET_COUNT;
	int nubucket = -1;
	unsigned int n, o;
	for(n=0; n<ADDRMAN_NEW_BUCKET_COUNT; n++){
		for(o=0; o<ADDRMAN_BUCKET_SIZE; o++){
			if(addrman->vv_new[n][o] == nid){
				nubucket = n;
			}
		}
		if(nubucket!=-1)
			break;
	}
	make_tried(addrman, tmp, nid, nubucket);
}

bool addrman_add_(struct addrman *addrman, struct links *links, struct link *source, int n_time_penalty){
	bool fnew = false;
	int nid = 0;
	//int	w, x;
	//struct caddrinfo *tried;
	struct caddrinfo *pinfo = find(addrman, links, &nid);
/*	for(w=0;addrman->v_random_size>50;w++){
		if(pinfo!=NULL)
			shrink_new(addrman, rand()%ADDRMAN_NEW_BUCKET_COUNT, pinfo->nid);
		else
			shrink_new(addrman, rand()%ADDRMAN_NEW_BUCKET_COUNT, (unsigned int)0-1);
		if(w==100){
			x = rand()%ADDRMAN_TRIED_BUCKET_COUNT;
			int n_pos = select_tried(addrman, x);
			tried = map_info(addrman, addrman->vv_tried[x][n_pos]);
			addrman->vv_tried[x][n_pos]=0;
			addrman->n_tried--;
			if(tried!=NULL){
				tried->f_in_tried=0;
				for(x=rand()%ADDRMAN_NEW_BUCKET_COUNT;;w++){
					if(addrman->vv_new[x][w]==0){
						addrman->vv_new[x][w]=tried->nid;
						break;
					}
				}
			}
			break;
		}
	}*/
	if(pinfo){
		bool f_currently_online = (sim_time - links->n_time < 24 * 60 * 60);
		unsigned int n_update_interval = (f_currently_online? 60 * 60 : 24 * 60 * 60);
		if(links->n_time&&(!pinfo->n_time || pinfo->n_time < links->n_time - n_update_interval - n_time_penalty))
			pinfo->n_time = max(0, links->n_time-n_time_penalty);
		if(!links->n_time || (pinfo->n_time && links->n_time <= pinfo->n_time))
			return false;
		if(pinfo->f_in_tried)
			return false;
		if(pinfo->n_ref_count == ADDRMAN_NEW_BUCKETS_PER_ADDRESS) //4
			return false;

		pinfo->n_time = max(0, pinfo->n_time - n_time_penalty);
		int n_factor = 1;
		int n;
		for(n=0; n < pinfo->n_ref_count; n++)
			n_factor *= 2;
		if(n_factor > 1 && rand()%n_factor==0)
			return false;
		pinfo->n_ref_count++;
	}
	else{
		pinfo = (struct caddrinfo*)create(addrman, links, source);
		pinfo->n_time = max(0, pinfo->n_time - n_time_penalty);
		addrman->n_new++;
		fnew = true;
	}
	int n_u_bucket		= get_new_bucket(pinfo, addrman);
	unsigned int *vnew	= &addrman->vv_new[n_u_bucket][0];
	int i;
	//for DEBUG
	for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
		if(vnew[i]==0){
			vnew[i] = pinfo->nid;
//			addrman->v_random_size++;
			return fnew;
		}
	}
	shrink_new(addrman, n_u_bucket, pinfo->nid);
	for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
		if(vnew[i]==0){
			vnew[i] = pinfo->nid;
			return fnew;
		}
	}
	return fnew;
}

void attempt(struct addrman *addrman, struct links *links, int n_time){
	int nid;
	struct caddrinfo *pinfo = find(addrman, links, &nid);
	if(!pinfo)
		return;
	struct caddrinfo *info = pinfo;
	
	if(info->new_comer!=links->new_comer)
		return;
	
	info->n_last_try = n_time;
	info->n_attempts++;
}

struct caddrinfo *addrman_select(struct addrman *addrman, int n_unk_bias){
	if(addrman->caddrinfo ==NULL)
		return NULL;
#ifdef ADDR_DEBUG
	fprintf(stderr, "in addrman_select(): n_tried = %d, n_new = %d\n", addrman->n_tried, addrman->n_new);
#endif
	unsigned int /*n,*/ num_try=0;
	double n_cor_tried	= sqrt(addrman->n_tried) * (100.0-n_unk_bias);
	double n_cor_new	= sqrt(addrman->n_new) * n_unk_bias;
	if(addrman->f_chance_factor<=0)
		addrman->f_chance_factor = 1.0;
	if((n_cor_tried + n_cor_new)*(rand()%(1 << 30)) / (1 << 30) < n_cor_tried){
//	unsigned int n;
//	for(n=0; n<ADDRMAN_TRIED_BUCKET_COUNT; n++){
//		fprintf(stderr, "vv_tried[%d][0] = %d\n", n, addrman->vv_tried[n][0]);
//	}

		//select from tried bucket
		while(1){
			num_try++;
			if(num_try>=2)
				return NULL;
			int n_k_bucket = rand()%ADDRMAN_TRIED_BUCKET_COUNT;
			int *vtried = (int *)&addrman->vv_tried[n_k_bucket][0];
			int size=0, i;
			for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
				if(vtried[i]!=0){
						size++;
				}
				else
					break;
			}
			if(size==0){
				num_try++;
				continue;
			}
			int n_pos = rand()%size;
			int nid = vtried[n_pos];
			struct caddrinfo *info = map_info(addrman, nid);
			if(info==NULL){
				num_try++;
				continue;
			}
//			fprintf(stderr, "addrman_select:\n addrman->f_chance_factor= %lf\n", addrman->f_chance_factor);
//			fprintf(stderr, "(addrman->f_chance_factor * get_chance(sim_time, info) * (1 << 30)) = %lf\n", (addrman->f_chance_factor * get_chance(sim_time, info) * (1 << 30)));
			if((rand()%(1 << 30)) < (addrman->f_chance_factor * get_chance(sim_time, info) * (1 << 30))){
				addrman->f_chance_factor = 1;
				return info;
			}
			addrman->f_chance_factor *= 1.2;//1.2;
			num_try++;
//			return NULL;
		}
	}
	else{
		//select from new bucket
		while(1){
			num_try++;
			if(num_try>=2)
				return NULL;
			int n_k_bucket = rand()%(ADDRMAN_NEW_BUCKET_COUNT);
			int (*vnew) = (int *)&addrman->vv_new[n_k_bucket][0];
			int size=0, i;
			for(i=0; i<ADDRMAN_BUCKET_SIZE; i++){
				if(vnew[i]!=0){
						size++;
				}
				else
					break;
			}
#ifdef ADDR_DEBUG
			fprintf(stderr, "addrman_select size = %d\n", size);
#endif
			if(size==0){
				num_try++;
				continue;
			}
			int n_pos = rand()%(size);
			int nid		= vnew[n_pos];
			struct caddrinfo *info = map_info(addrman, nid);
			if(info==NULL){
				num_try++;
				continue;
			}
//			fprintf(stderr, "addrman_select: in if= %lf\n", (addrman->f_chance_factor * get_chance(sim_time, info) * (1 << 30)));
			if((rand()%(1 << 30))< (addrman->f_chance_factor * get_chance(sim_time, info) * (1 << 30))){
				addrman->f_chance_factor = 1;
				return info;
			}
			addrman->f_chance_factor *= 1.2;//1.2;
			num_try++;
//			return NULL;
		}
	}
}
unsigned int getaddr_(struct addrman *addrman, unsigned char *vaddr){//will fix vaddr
	unsigned int n_nodes = ADDRMAN_GETADDR_MAX_PCT * addrman->v_random_size / 100;
	if(n_nodes > ADDRMAN_GETADDR_MAX)
		n_nodes = ADDRMAN_GETADDR_MAX;
	if(n_nodes>(unsigned int)(BUF_SIZE-16)/(sizeof(unsigned int)*3+sizeof(struct link *))-5)
		n_nodes = (unsigned int)(BUF_SIZE-16)/(sizeof(unsigned int)*3+sizeof(struct link *))-5;
	//gather list of random nodes, skipping low quality ones
	unsigned int n, i, sets;
#ifdef ADDR_DEBUG 
	fprintf(stderr, "addrman->v_random_size = %d\n", addrman->v_random_size);
#endif
#ifdef ADDR_DEBUG
	for(n=0; n<addrman->v_random_size; n++){
		fprintf(stderr, "vrandom[%d] = %d\n", n, addrman->v_random[n]);
	}
	fprintf(stderr, "v_random_size = %d\n", addrman->v_random_size);
#endif
	i=0;
	sets = sizeof(unsigned int)*3 + sizeof(struct link*);
	for(n=0; n < addrman->v_random_size && n*sets<BUF_SIZE-16; ){
		if(n >= n_nodes)
			break;
		int n_rnd_pos = (rand()%(addrman->v_random_size - n)) + n;
#ifdef ADDR_DEBUG
//		fprintf(stderr, "n = %d, n_rnd_pos = %d, v_random_size = %d\n", n, n_rnd_pos, addrman->v_random_size);
#endif
		swap_random(addrman, n, n_rnd_pos);
		struct caddrinfo *ai = map_info(addrman, addrman->v_random[n]);
		if(ai!=NULL){
			if(!is_terrible(sim_time, ai)){
				add(&vaddr[i*sets], ai);//will fix vaddr
				i++;
			}
		}
		n++;
	}
//	fprintf(stderr, "getaddr_ sending i = %d addrs\n", i);
	return i;
}

void connected(struct addrman *addrman, struct links *links, int n_time){
	int i=0;
	struct caddrinfo *pinfo = find(addrman, links, &i);
	if(!pinfo)
		return;
	
	struct caddrinfo *info = pinfo;
	if(info->new_comer != links->new_comer)
		return;
	int n_update_interval = 20*60;
	if(n_time - info->n_time > n_update_interval)
		info->n_time = n_time;
}
void add_fixed_seeds(struct addrman *addrman){
	unsigned int i;
	struct links tmp;
	for(i=0; i<SEED_NUM; i++){
// seeds[i].miner_id, &seeds[i].new_comer, &seeds[i].new_comer,
		tmp.miner_id	= seeds[i]->miner_id;
		tmp.new_comer	= &seeds[i]->new_comer;
		tmp.subnet		= seeds[i]->subnet;
		
		addrman_add_(addrman, &tmp, NULL, 0);
//		fprintf(stderr, "tmp.subnet=%d\n", tmp.subnet);
//bool addrman_add_(struct addrman *addrman, struct links *links, struct link *source, int n_time_penalty){
	}
}
#endif
