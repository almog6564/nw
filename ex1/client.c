#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "nw.h"
#include "message.h"


// got size of wellcome but not message
int clientProtocol(SOCKET);

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
    if(welcome.opcode != WELLCOME){
        printf("Not a wellcome message\n");
        return -1;
    }
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

    status = clientProtocol(s);

    
    //status = startService(s);

    if(close(s)<0){
        printf("close error\n");
       return -1;
    }

    return status;

}


int userLogin(SOCKET s){
    int  status = 0;
    char input[MAX_LOGIN];
    char username[50];
    char password[50];

    /*USERNAME*/
    memset(input,0,MAX_LOGIN);
    fflush(stdout);
    fgets(input,MAX_LOGIN,stdin);
    input[strlen(input)-1] = '\0';
        
    if(!strncmp(input,"User: ",)){
        strcpy(username,input+6);
    } else{
        printf("Error while getting username");
        return -1;
    }
    /***/

    /*PASSWORD*/
    memset(input,0,MAX_LOGIN);
    fflush(stdout);
    fgets(input,MAX_LOGIN,stdin);
    input[strlen(input)-1] = '\0';
        
    if(!strncmp(input,"Password: ",)){
        strcpy(password,input+10);
    } else{
        printf("Error while getting password");
        return -1;
    }
    /***/
    MSG loginMsg;
    loginMsg.opcode = LOGIN;
    loginMsg.length = strlen(username)+strlen(password)+2;
    strncpy(loginMsg.msg,username,strlen(username)+1);
    strncpy(loginMsg.msg+strlen(username)+1,password,strlen(password)+1);

    status = sendMessage(s,&loginMsg);
    if(status<0){
        printf("Error while sending login message");
        return -1;
    }

    MSG loginStatus;
        
    status = getMessage(s, &loginStatus);
    if(status < 0)
    {
        printf("Error while getting login status\n");
        return -1;
    }

    if(loginStatus.opcode != LOGIN_SUCCESS){
        printf("Connected to server\n");
        return 0;
    } 

    if(loginStatus.opcode != LOGIN_FAIL){
        printf("Username or password incorrect\n");
    else
        printf("Error while getting login status\n");
        
    return -1;

}


int clientProtocol(SOCKET s){
    int  status = 0;
    char input[MAX_INPUT];
    
    status = getWelcome(s); 

    if(status < 0){
        printf("getWelcome error\n");
        return -1;
    }

    status = userLogin(s);
    if(status < 0){
        printf("Error while login\n");
        return -1;
    }
    while(1){
        memset(input,0,MAX_INPUT);
        fflush(stdout);
        fgets(input,MAX_INPUT,stdin);
        input[strlen(input)-1] = '\0';
        



    }


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