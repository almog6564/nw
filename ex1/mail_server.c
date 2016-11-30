#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "message.h"

USERLIST lst;

int createUsersList(char* path){
    memset(&lst,0,sizeof(lst));
    char line[LINE_LEN];
    int i = 0;    
    FILE* fp = fopen(path, "r");
    if(!fp){
        printf("Error while opening users file");
        return -1;
    }
    while (fgets(line, sizeof(line), fp) != NULL && i < MAX_USERS){
        sscanf(line, "%s %s", lst.list[i].username, lst.list[i].password); // TODO Error?
        lst.list[i].inbox = (MAIL*)malloc(sizeof(MAIL));
        lst.list[i].inboxSize = 1;
        lst.list[i].inboxUsed = 0;
        i++;
        lst.size++;
    }
    fclose(fp);
    return 0;
}

int login(SOCKET s) {
	int i;
	MSG logMsg, connected;
	USER usr;
	char username[MAX_LEN], password[MAX_LEN];
	int status = getMessage(s, &logMsg);
	if (status < 0) {
		return -1; // ERROR
	}
	strcpy(username, logMsg.msg);
	strcpy(password, logMsg.msg + strlen(username) + 1);

	for (i = 0; i < lst.size; i++) {
		usr = lst.list[i];
		if (strcmp(usr.username, username) == 0 && strcmp(usr.password,
				password) == 0) {
			break;
		}
		if (i == lst.size - 1) {
			// Send Error MSG
			return -1;
		}
	}

	connected.opcode = LOGIN_SUCCESS;
	connected.length = 0;
	if (sendMessage(s, &connected) < 0) {
		printf("sendMessage error\n");
		return -1;
	}
	return 0;

}

int receiveMail(SOCKET s){
	int i;
	MAIL mail;
	MSG mailMsg, mailSent;
	USER usr;
	char username[MAX_LEN], subject[MAX_SUBJECT], content[MAX_CONTENT];
	int status = getMessage(s,&mailMsg);
	if (status<0){
	    return -1; // ERROR
	}
	//extract the parameters of the mail from the msg
	strcpy(username,mailMsg.msg);//TODO - check if multiple users
	strcpy(subject,mailMsg.msg+strlen(username)+1);
	strcpy(content,mailMsg.msg+strlen(username)+1+strlen(subject)+1);

	// insert the parameters from the msg to a new mail
	/*
	mail.from = ;
	mail.to = username;
	mail.subject = subject;
	mail.text = content;
	*/
	//search for the recipient of the mail and insert the new mail to its inbox
	for (i=0; i<lst.size; i++){
	    usr = lst.list[i];
	    if (strcmp(usr.username,username)==0){
	    	if (usr.inboxSize == 0){
	    		usr.inbox[0] = mail;
	    		usr.inboxUsed++;
	    		break;
	    	}
	    	else {
	    		if (usr.inboxSize>usr.inboxUsed) // if there is allocated space for the mail
	    			usr.inbox[usr.inboxUsed++] = mail;
	    		else { // if there is none allocated space, do doubling
	    			usr.inboxSize *= 2;
	    			usr.inbox = (MAIL*)realloc(usr.inbox, usr.inboxSize*sizeof(MAIL));
	    		}
	    		break;
	    	}
	    }
	    else if (i==lst.size-1){
	        // Send Error MSG
	        return -1;
	    }
	}
	mailSent.opcode = COMPOSE;
	mailSent.length = 0;
	if(sendMessage(s,&mailSent)<0){
	    printf("sendMessage for mailSent error\n");
	    return -1;
	}
	return 0;

}

int serverProcess(SOCKET s) {
	MSG m;

	//welcome and login will always be on the start

	m.length = WELCOME_SIZE;
	m.opcode = WELCOME;
	strncpy(m.msg, WELCOME_MSG, m.length);

	// hello string message
	if (sendMessage(s, &m) < 0) {
		printf("Error while sending welcome message\n");
		return -1;
	}

	if (login(s) < 0) {
		printf("error in login - wrong username/password\n");
		return -1;
		//Error in login
	}
	while (1) {	//Main commands loop. suppose to continue until getting QUIT opcode
		MSG get;
		if (getMessage(s, &get) < 0) {
			printf("Error while getting message on server\n");
			return -1;
		}
		switch (get.opcode) {

		case QUIT:
			close(s);
			printf("DEBUG - Got quit message\n");
			return QUIT;

		case SHOW_INBOX:
			printf("DEBUG - Got show inbox\n");
			return 0;

		case GET_MAIL:
			printf("DEBUG - Got get mail\n");
			return 0;
		}
	}

	return 0;
}

int initServer(int port) {

	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		printf("socket error\n");
		return -1;
	}

	struct sockaddr_in my_addr, client_addr;

	struct in_addr _addr;
	_addr.s_addr = htonl(INADDR_ANY);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr = _addr;

	if (bind(sock, (struct sockaddr *) (&my_addr), sizeof(my_addr)) < 0) {
		printf("bind error\n");
		return -1;
	}

	if (listen(sock, 10) < 0) {
		printf("listen error\n");
		return -1;
	}

	socklen_t sin_size = sizeof(struct sockaddr_in);

	//maybe need to be in a loop:

	while (1) {		//Main client connecting loop. after a quit waits for another accept
		SOCKET s = accept(sock, (struct sockaddr *) (&client_addr), &sin_size);
		if (s < 0) {
			printf("Client accept error\n");
			return -1;
		}

		int status = serverProcess(s);
		if (status < 0) {
			return -1;
		} else if (status == QUIT) {
			continue;
		} else
			break;

	}

	return 0;

}

int main(int argc, char* argv[]) {

	//check arguments
	int status = 0;
	int port = DEFAULT_PORT;
	switch (argc) {

	case 3:
		port = atoi(argv[2]);

	case 2:
		status = createUsersList(argv[1]);
		if (status < 0) {
			printf("Failed to get users list\n");
			return -1;
		}

		for (int i = 0; i < lst.size; i++) {
			printf("User: %s,\tPassword: %s\n", lst.list[i].username,
					lst.list[i].password);
		}

		break;

	default:
		printf("Invalid arguments.\nTry: mail_server users_file [port]\n");
		return -1;

	}

	SOCKET s = initServer(port);
	if (s < 0) {
		printf("Server Error\n");
		return -1;
	}

	return status;

}
