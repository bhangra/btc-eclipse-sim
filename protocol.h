#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/sem.h>

#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
modifying bitcoin's header without
"magic number" and "checksum"

Size	| Description	| Data Type		| Comment
---------------------------------------------------------
12		| command		| char[12]		| ascii string
4		| length		| unsigned int	| length of payload
?		| payload		| char[]		| 

*/
struct msg_hdr{
	char			command[12];
	unsigned int	message_size;
};

/*
addr message header
Size	| Description	| Data Type		| Comment
----------------------------------------------------------
4		| count			| unsigned int	| Num of Addresses
?		| addr_list		| net_addr		| Pointers of other nodes

addr_list
Size    | Description   | Data Type     | Comment
----------------------------------------------------------
todo!!!!!!!!!!!!!!!!


*/


#endif
