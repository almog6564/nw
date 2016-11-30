#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "nw.h"
#include "message.h"

#define readVarFromString(string,string_size,output, prefix,prefix_size,error_msg)		\
																				\
									memset(string, 0, string_size);			\
									fflush(stdout);								\
									fgets(string, string_size, stdin);			\
									string[strlen(string) - 1] = '\0';		\
																				\
									if (!strncmp(string, prefix, prefix_size)) {	\
									strcpy(output, string + prefix_size);				\
									} else {									\
									printf(error_msg);	\
									return -1;									\
									}

USER user;

// got size of welcome but not message
int clientProtocol(SOCKET);

int getWelcome(SOCKET s) {

	MSG welcome;

	int status = getMessage(s, &welcome);
	if (status < 0) {
		return -1;
	}
	//check if welcome is correct

	welcome.msg[welcome.length - 1] = '\0'; //sanity
	if (welcome.opcode != WELCOME) {
		printf("Welcome message is invalid\n");
		return -1;
	}
	printf("%s\n", welcome.msg);
	return 0;

}

int initClient(char* hostname, int port) {

	int status = 0;
	struct addrinfo hints;
	struct addrinfo *serverinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char portString[8];

	sprintf(portString, "%d", port);

	//printf("portString: %s\n",portString);

	if (getaddrinfo(hostname, portString, &hints, &serverinfo) < 0) {
		printf("getaddrinfo\nError: %s\n", strerror(errno));
		return -1;
	}

	//printf("got address\n");

	SOCKET s = socket(serverinfo->ai_family, serverinfo->ai_socktype,
			serverinfo->ai_protocol);

	//printf("got socket\n");

	if (s < 0) {
		printf("socket\n");
		return -1;
	}

	//maybe TODO loop: ?
	if (connect(s, serverinfo->ai_addr, serverinfo->ai_addrlen)) {
		printf("connect error\n");
		return -1;
	}

	//printf("got connect\n");

	freeaddrinfo(serverinfo);

	//printf("got freeaddrinfo\n");

	status = clientProtocol(s);

	//status = startService(s);

	if (close(s) < 0) {
		printf("close error\n");
		return -1;
	}

	return status;

}

int userLogin(SOCKET s) {
	int status = 0;
	char input[MAX_LOGIN];
	char username[MAX_LEN];
	char password[MAX_LEN];

	/*USERNAME*/readVarFromString(input,MAX_LOGIN,username,"User: ",6,"Error while getting username\n")

	/*PASSWORD*/readVarFromString(input,MAX_LOGIN,password,"Password: ",10,"Error while getting password\n")

	strcpy(user.username, username);
	strcpy(user.password, password);

	MSG loginMsg;
	loginMsg.opcode = LOGIN;
	loginMsg.length = strlen(username) + strlen(password) + 2;
	strncpy(loginMsg.msg, username, strlen(username) + 1);
	strncpy(loginMsg.msg + strlen(username) + 1, password, strlen(password) + 1);

	status = sendMessage(s, &loginMsg);
	if (status < 0) {
		printf("Error while sending login message\n");
		return -1;
	}

	MSG loginStatus;

	status = getMessage(s, &loginStatus);
	if (status < 0) {
		printf("Error while getting login status\n");
		return -1;
	}

	if (loginStatus.opcode == LOGIN_SUCCESS) {
		printf("Connected to server\n");
		return 0;
	} else if (loginStatus.opcode == LOGIN_FAIL)
		printf("Username or password incorrect\n");
	else
		printf("Error while getting login status\n");

	return -1;

}

int showInbox(SOCKET s) {
	int status = 0;
	MSG cmnd;
	cmnd.opcode = SHOW_INBOX;
	cmnd.length = strlen(user.username) + 1;
	strcpy(cmnd.msg, user.username);

	status = sendMessage(s, &cmnd);
	if (status < 0) {
		printf("Sending show Inbox command failed\n");
		return -1;
	}
	MSG inbox;
	status = getMessage(s, &inbox);
	if (status < 0) {
		printf("Getting message command failed\n");
		return -1;
	}
	if (inbox.length > 0)
		printf("%s", inbox.msg);
	else {
		printf("Error while showing inbox\n");
		return -1;
	}

	return 0;
}

int getMail(SOCKET s, char* mail_ID) {
	int status = 0;
	MSG cmnd;
	cmnd.opcode = GET_MAIL;
	cmnd.length = strlen(user.username) + strlen(mail_ID) + 2;
	strcpy(cmnd.msg, user.username);
	strcpy(cmnd.msg + strlen(user.username) + 1, mail_ID);
	status = sendMessage(s, &cmnd);

	if (status < 0) {
		printf("Sending getMail command failed\n");
		return -1;
	}
	MSG mailMSG;
	status = getMessage(s, &mailMSG);
	if (status < 0) {
		printf("Getting message command failed\n");
		return -1;
	}
	if (mailMSG.opcode == INVALID) {
		printf("Invalid mail ID.\n");
		return 0;
	}
	/*
	 MAIL* mail = (MAIL*) mailMSG.msg;
	 printf("From: %s\n");
	 printf("To: \n")
	 for(int i=0;i<mail.toLen;i++){
	 printf("%s",mail.to[i]);
	 if(i != mail.toLen -1)
	 printf(",");
	 }

	 printf("Subject: %s\nText: %s\n",mail.subject,mail.text);
	 */
	return 0;

}

void getRecipients(char to[], MAIL* pmail) {
	short toLen = 1;
	char* cpy = to;

	for (; *cpy != '\0'; cpy++) {
		if (*cpy == ',') {
			toLen++;
			*cpy = 0;
		}
	}
	pmail->toLen = toLen;

	cpy = to;

	for (int i = 0; i < toLen; i++) {
		strcpy(pmail->to[i], cpy);
		cpy += (strlen(cpy) + 1);
	}

}

int composeMail(SOCKET s) {
	int status = 0;
	char to[MAX_COMPOSE_TO]; //enough for commas
	char subject[MAX_SUBJECT];
	char text[MAX_CONTENT];
	MAIL mail;
	strcpy(mail.from, user.username);

	/*to*/
	memset(to, 0, MAX_COMPOSE_TO);
	fflush(stdout);
	fgets(to, MAX_COMPOSE_TO, stdin);
	to[strlen(to) - 1] = '\0';

	do {
		status = strncmp(to, "To: ", 4);
		if (status < 0)
			break;
		if (*(to + 4) == 0) {
			printf("Message must have recipients\n");
			status = -1;
			break;
		}
		getRecipients(to + 4, &mail);
		break;
	} while (0);
	if (status < 0) {
		printf("Error while getting recipients\n");
		return -1;
	}
	/***/

	//subject
	readVarFromString(subject,MAX_SUBJECT,mail.subject,"Subject: ",9,"Error while getting subject\n")

	//text
	readVarFromString(text,MAX_CONTENT,mail.text,"Text: ",6,"Error while getting text\n")

	MSG mailMSG;
	mailMSG.length = sizeof(MAIL);
	mailMSG.opcode = COMPOSE;
	memcpy(mailMSG.msg, &mail,sizeof(MAIL));

	/*
	if(sendMessage(s,&mailMSG) < 0){

	}
	*/
	return 0;
}

int sendQuit(SOCKET s) {
	MSG quit;
	quit.opcode = QUIT;
	quit.length = 0;
	if (sendMessage(s, &quit) < 0) {
		printf("Error while sending quit message\n");
		return -1;
	}
	return 0;
}

int clientProtocol(SOCKET s) {
	int status = 0;
	char input[MAX_INPUT];

	status = getWelcome(s);

	if (status < 0) {
		printf("getWelcome error\n");
		return -1;
	}

	status = userLogin(s);
	if (status < 0) {
		printf("Error while login\n");
		return -1;
	}

	while (1) {
		memset(input, 0, MAX_INPUT);
		fflush(stdout);
		fgets(input, MAX_INPUT, stdin);
		input[strlen(input) - 1] = '\0';
		if (!strcmp(input, "SHOW_INBOX")) {
			if (showInbox(s) < 0) {
				printf("Error while getting inbox\n");
				return -1;
			}
		} else if (!strncmp(input, "GET_MAIL", 8)) {
			if (strlen(input) < 10) {
				UnknownCommand()
			}
			if (getMail(s, input + 9)) {
				printf("Error while getting mail\n");
				return -1;
			}
		} else if (!strncmp(input, "COMPOSE", 8)) {
			if (composeMail(s) < 0) {
				printf("Error while composing mail\n");
				return -1;
			}

		} else if (!strncmp(input, "QUIT", 4)) {
			if (sendQuit(s) < 0) {
				printf("Error while sending quit\n");
				return -1;
			}
			return 0;
		}
	}
}

int main(int argc, char* argv[]) {

	int status = 0;

	char hostname[MAX_LEN];
	int port = 0;

	switch (argc) {
	case 1:
		strcpy(hostname, HOST);
		port = DEFAULT_PORT;

		break;

	case 3:
		port = atoi(argv[2]);
		if (!port)
			goto INVALID;

	case 2:
		strcpy(hostname, argv[1]);

		break;

	default:
		INVALID: printf(
				"Invalid arguments.\nTry: mail_client [hostname [port]]");
		return -1;

	}
	status = initClient(hostname, port);
	if (status < 0) {
		printf("initCLient error\n");
	}

	return status;
}
