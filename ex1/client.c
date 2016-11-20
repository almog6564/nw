#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>


#include "nw.h"
#include "message.h"


// got size of wellcome but not message


int getWelcome(SOCKET s){

    MSG welcome;
    
    
    int status = getMessage(s, &welcome);
    if(status < 0)
    {
        printf("error getMessage\n");
        return -1;
    }
    //check if welcome is correct

    welcome.msg[welcome.length - 1] = '\0';       //sanity

    printf("%s\n",welcome.msg);
    return 0;

}


int initClient(char* hostname,int port){

    int	status = 0;
    struct addrinfo hints;
    struct addrinfo *serverinfo; 

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
  
  
    char portString[8];

    sprintf(portString,"%d",port);

    //printf("portString: %s\n",portString);

    if(getaddrinfo(hostname, portString, &hints, &serverinfo) < 0){
        printf("getaddrinfo\nError: %s\n",strerror(errno));
        return -1;
    }
    
    //printf("got address\n");

    SOCKET s = socket(serverinfo->ai_family,serverinfo->ai_socktype,serverinfo->ai_protocol);
    
    //printf("got socket\n");

    if(s < 0){
        printf("socket\n");
		return -1;
	}

    //maybe TODO loop: ?
    if(connect(s,serverinfo->ai_addr,serverinfo->ai_addrlen)){
        printf("connect error\n");
        return -1;
    }
    
    //printf("got connect\n");

    freeaddrinfo(serverinfo);

    //printf("got freeaddrinfo\n");

    status = getWelcome(s);

    if(status < 0){
        printf("getWelcome error\n");
        return -1;
    }
    //status = startService(s);

    if(close(s)<0){
        printf("close error\n");
       return -1;
    }

    return status;

}




int main(int argc, char* argv[]){
	
	//check arguments
	
    int status = 0;
/*
	switch(argc){
        case 1:
        status = initClient(HOST,PORT);
        //hostname = localhost
        //port = 6423

		case 2:
        status = initClient(argv[1],PORT);
        if(status < 0){
            //error
            return -1;
        }
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
    status = initClient(HOST,PORT);
    if(status < 0){
        printf("initCLient error\n");
    }
	
	
	return status;
}