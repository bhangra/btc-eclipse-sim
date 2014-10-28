#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#include"block.h"
#include"connection.c"


void *dns_query(){
    
}

void version(){
}
double verack(){
}

int process_msg(char *msg_ptr){
	const struct msg_hdr *hdr;
	hdr = msg_ptr;
	if(strncmp(hdr->command, "addblock", 12)){
		
	}
	else if(strncmp(hdr->command, "block", 12)){
		
	}
	else if(strncmp(hdr->command, "newhead", 12)){

	}
	else if(strncmp(hdr->command, "getblock", 12)){

	}
	else if(strncmp(hdr->command, "roundrobin", 12)){

	}
	else if(strncmp(hdr->command, "getaddr", 12)){
		
	}
	else if(strncmp(hdr->command, "addr", 12)){

	}
	else if(strncmp(hdr->command, "version", 12)){
		
	}
	else if(strncmp(hdr->command, "verack", 12)){

	}
}

void mining_thread(void *param){
	struct links links;
	struct link	link;
	double x, y;
	int mined;
	int times;
	
	links.link = &link;

	while(1){
		for(times = 0;times < 1000;times++){
			x = rand()/(RAND_MAX);
			y = rand()/(RAND_MAX);
			if((x*x+y*y)>0.5){
				mined = 1;
				printf("mined block in %d times\n", times);
				break;
			}
		}
		
	}
}


