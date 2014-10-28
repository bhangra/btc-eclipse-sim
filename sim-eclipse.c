#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>

#include"node.c"

int main(int argc, char *argv[]){
	pthread_t thread;
	int pid;
	pid = getpid();
	if(pthread_create(&thread, NULL, mining_thread, NULL)!=0){
		perror("pthread_create()");
		return -1;
	}
	pthread_join(thread, NULL);
	return EXIT_SUCCESS;
}
