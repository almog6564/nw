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

int createUsersList(char* path) {
	memset(&lst, 0, sizeof(lst));
	char line[LINE_LEN];
	int i = 0;
	FILE* fp = fopen(path, "r");
	if (!fp) {
		printf("Error while opening users file");
		return -1;
	}
	while (fgets(line, sizeof(line), fp) != NULL && i < MAX_USERS) {
		sscanf(line, "%s %s", lst.list[i].username, lst.list[i].password); // TODO Error?
		i++;
		lst.size++;
	}
	fclose(fp);
	return 0;
}

int deleteMail(SOCKET s,MSG* delmsg){
	//search for user
	int usr = 0;
	for (; usr < lst.size; usr++) {
		if(!strcmp(delmsg->msg, lst.list[usr].username)){
			break;
		}
	}
	int mailID = atoi(delmsg->msg+strlen(delmsg->msg)+1);
	if(mailID < 0 || mailID >= MAXMAILS){
		MSG inv;
		inv.opcode = INVALID;
		inv.length = 0;
		if(sendMessage(s,&inv)<0){
			printf("Error while sending invalid message in deleteMail\n");
			return -1;
		}
	}

	//Deleting:
	//If mail doesnt exist it doesnt matter
	memset(&lst.inbox[usr][mailID],0,sizeof(MAIL));

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
			MSG inv;
			inv.opcode = LOGIN_FAIL;
			inv.length = 0;
			if(sendMessage(s,&inv)<0){
				printf("Error sending invalid message\n");
				return -1;
			}
			return 0;
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

int receiveMail(SOCKET s, MSG* _mailMsg) {
	int i = 0, j = 0;
	MAIL mail;
	MSG mailSent, opInval;
	MSG mailMsg = *_mailMsg;

	memcpy(&mail, (MAIL*)mailMsg.msg, sizeof(MAIL));

	//extract the parameters of the mail from the msg
	for (; i < mail.toLen; i++) {
		for (; j<MAX_USERS; j++) {
			if (!strcmp(lst.list[j].username, mail.to[i]))
				break;
			else if (j==MAX_USERS-1){
				opInval.opcode = INVALID;
				opInval.length = 0;
				sendMessage(s,&opInval);
			}
		}
		memcpy(&(lst.inbox[j][lst.inboxSizes[j]]), &mail, sizeof(MAIL));
		lst.inboxSizes[j]++;
		j = 0;
	}

	mailSent.opcode = COMPOSE;
	mailSent.length = 0;
	if (sendMessage(s, &mailSent) < 0) {
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
	while (1) { //Main commands loop. suppose to continue until getting QUIT opcode
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

		case COMPOSE:
			if(receiveMail(s, &get)< 0){
				printf("Error while getting compose message\n");
				return -1;
			}
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

	while (1) { //Main client connecting loop. after a quit waits for another accept
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
