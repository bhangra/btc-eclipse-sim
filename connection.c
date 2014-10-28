#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<pthread.h>
#include<unistd.h>

#include"connection.h"

#define HDR_SIZE	16

int send_msg(struct link *dest, char *message, int msg_size){
	pthread_mutex_lock((pthread_mutex_t *)&dest->rcv_mutex);
/*	if(){
	}
	else(){
		
	}
*/
	dest->num_msg += 1;
	pthread_mutex_unlock((pthread_mutex_t *)&dest->rcv_mutex);
}

int read_msg(struct link *link){
	const struct msg_hdr	*hdr;
	int						read_size;
	int						over_size;
	if(!(int)link->num_msg){
		return 0;
	}
	pthread_mutex_lock(&link->rcv_mutex);
	hdr 		= (struct msg_hdr*)&link->buf[(link)->read_pos];
	read_size	= HDR_SIZE + hdr->message_size;
//assume that the BUF_SIZE is large enough for a message
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
//	process_msg(link->process_buf);
	return 1;
}

/*
int readMessage(struct Connection connetion, const char *pch, unsigned int nBytes){
	unsigned int nRemaining = 24 -nHdrPos;
	unsigned int nCopy		= (nRemainning>nBytes) nRemaining : nBytes;

	memcpy(message.hdrbuf[nHdrPos], message.pch, nCopy);
	nHdrPos 	+= nCopy;
	//header incomplete -> exit
	if(nHdrPos < 24)
		return nCopy;
	memcpy(message.hdr[nHdrPos] , &hdrbuf[nHdrPos], nCopy);
	if(hdr.nMessageSize > MAX_SIZE)
		return -1;

	message->in_data = true;
	

	return nCopy;
	 
}
*/
