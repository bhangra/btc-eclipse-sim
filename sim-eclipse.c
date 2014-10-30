#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#include"thread.c"
#include"node.c"
#include"routine.c"

struct link		dns_link[5];
struct links 	dns_links[5];


int main(int argc, char *argv[]){
/*	pthread_t thread;
	if(pthread_create(&thread, NULL, mining_thread, NULL)!=0){
		perror("pthread_create()");
		return -1;
	}
	pthread_join(thread, NULL);
*/
	int time;
	struct threads *thread;
	thread = new_thread(0, NULL);
	for(time = 0; time < 5; time++){
		fprintf(stderr, "time = %d\n", time);//debug
		miner_routine(thread->miner);
	}
	cancel_all(thread);
	return EXIT_SUCCESS;
}
