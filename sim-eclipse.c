#ifndef SIM_ECLIPSE_C
#define SIM_ECLIPSE_C


#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

//#include"thread.c"
//#include"node.c"
#include"routine.c"


#define BAD_DNS 0
#define ATTACKER 0
#define INIT_NODES 1000

int main(int argc, char *argv[]){
//	struct dns	dns[5];
	unsigned int time, miner_id, i;
	struct threads *threads;

//DNS nodes initialization
	memset(&dns, 0, 5*sizeof(struct dns));


	threads = NULL;
	for(miner_id=0; miner_id<3; miner_id++){
		threads=new_thread(1, miner_id, threads);
	}
//	thread = new_thread(1, NULL);
	for(time = 0; time < 20; time++){
		fprintf(stderr, "time = %d\n", time);//debug
		for(;threads->prev!=NULL; threads=threads->prev){}
		for(;;threads=threads->next){
			miner_routine(threads->miner);
			if(threads->next==NULL)
				break;
		}
		for(i=0; i<5; i++){
			dns_routine(&dns[i]);
		}
	}
	cancel_all(threads);
	return EXIT_SUCCESS;
}

#endif
