#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#include"thread.c"
#include"node.c"

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
	struct threads *thread, *head, *tail;
	thread = new_thread(mining_thread, NULL);
	if(thread != NULL){
		head = thread;
		tail = thread;
	}
	else{
		perror("new_thread()");
		exit(-1);
	}
	sleep(5);
	cancel_all(thread);
	return EXIT_SUCCESS;
}
