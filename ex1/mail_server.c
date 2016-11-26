#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "message.h"

#define MAX_LEN 50
#define MAX_USERS 20
#define LINE_LEN 1000

typedef struct {
    char username[MAX_LEN];
    char password[MAX_LEN];
} USER;

USER usersList[MAX_USERS];

int createUsersList(cahr* path){
    char line[LINE_LEN];
    int i = 0;    
    File* fp = fopen(path, "r");
    
    while (fgets(line, sizeof(line), file) != NULL){
        sscanf(line, "%s %s", usersList[i].username, usersList[i].password); // TODO Error?
        i++;
    {
    
    return 0;
}

int initServer(int port){

    SOCKET sock = socket(PF_INET,SOCK_STREAM,0);

    if(sock<0){
		printf("socket error\n");
        return -1;
	}
	
	struct sockaddr_in  my_addr, client_addr;

    struct in_addr      _addr;
    _addr.s_addr       = htonl(INADDR_ANY);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port   = htons(port);
    my_addr.sin_addr   = _addr;

    if(bind(sock,(struct sockaddr *)(&my_addr),sizeof(my_addr))<0){
         printf("bind error\n");
         return -1;
    }

    if(listen(sock,10)<0){
         printf("lsiten error\n");
         return -1;
    }

    socklen_t sin_size      = sizeof(struct sockaddr_in);

    //maybe need to be in a loop:
    int client_socket = accept(sock,(struct sockaddr *)(&client_addr),&sin_size);

    if(client_socket<0){
         printf("accept error\n");
         return -1;
    }

    return client_socket;

}




int main(int argc, char* argv[]){
	
	//check arguments
	int status = 0;
    int port = PORT;

/*
	switch(argc){
		case 2:
		//only users_file;
		break;
		
		case 3:
		//users_file + port
		break;
		
		default:
		//wrong arguments
		break;
		
			
	}
*/

	SOCKET s = initServer(port);
	if(s < 0){
        printf("initServer error");
        return -1;
}
    //we have a client!


	MSG m;
    //memset(m,0,sizeof(MSG));
	m.length = WELLCOME_SIZE;
    m.opcode = WELLCOME;
	strncpy(m.msg, WELLCOME_MSG, m.length);

	// hello string message
	status = sendMessage(s, &m);
    
	return status;
}