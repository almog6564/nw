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

#define OK 	  (0)
#define ERROR (-1)

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
									return ERROR;									\
									}

USER user;


int clientProtocol(SOCKET);

/*
 * This function gets the welcome message from the server and prints it to the screen.
 * param s	-	SOCKET of the connection
 * return ERROR if sending error or connection ended,
 	 	  otherwise return OK
 */
int getWelcome(SOCKET s) {

	MSG welcome;

	int status = getMessage(s, &welcome);
	if (status < 0) {
		return ERROR;
	}
	//check if welcome is correct

	welcome.msg[welcome.length - 1] = '\0'; //sanity
	if (welcome.opcode != WELCOME) {
		printf("Welcome message is invalid\n");
		return ERROR;
	}
	printf("%s\n", welcome.msg);
	return OK;

}

/*
 * This function runs the main flow of the client connection process and calls
 * function clientProtocol for main flow of commands.
 * param hostname	-	hostname to be used for connection
 * param port		-	port to be used for connection
 * return ERROR if sending error or connection ended,
 	 	  otherwise return OK
 */
int initClient(char* hostname, int port) {

	int status = OK;
	struct addrinfo hints;
	struct addrinfo *serverinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char portString[8];

	sprintf(portString, "%d", port);

	if (getaddrinfo(hostname, portString, &hints, &serverinfo) < 0) {
		printf("getaddrinfo\nError: %s\n", strerror(errno));
		return ERROR;
	}

	SOCKET s = socket(serverinfo->ai_family, serverinfo->ai_socktype,
			serverinfo->ai_protocol);

	if (s < 0) {
		printf("socket\n");
		return ERROR;
	}

	if (connect(s, serverinfo->ai_addr, serverinfo->ai_addrlen)) {
		printf("connect error\n");
		return ERROR;
	}

	freeaddrinfo(serverinfo);

	status = clientProtocol(s);

	if (close(s) < 0) {
		printf("close error\n");
		return ERROR;
	}

	return status;

}

/*
 * This function gets the login message from the user and connects
 * param hostname	-	hostname to be used for connection
 * param port		-	port to be used for connection
 * return ERROR if sending error or connection ended,
 	 	  otherwise return OK
 */
int userLogin(SOCKET s) {
	int status = OK;
	char input[MAX_LOGIN];
	char username[MAX_LEN];
	char password[MAX_LEN];

	//username
	readVarFromString(input,MAX_LOGIN,username,"User: ",6,"Error while getting username\n")

	//password
	readVarFromString(input,MAX_LOGIN,password,"Password: ",10,"Error while getting password\n")

	strcpy(user.username, username);
	strcpy(user.password, password);

	int uslen = strlen(username) + 1, pslen = strlen(password) + 1;

	MSG loginMsg;
	loginMsg.opcode = LOGIN;
	loginMsg.length = uslen + pslen;

	strncpy(loginMsg.msg, username, uslen);
	strncpy(loginMsg.msg + uslen, password, pslen);

	status = sendMessage(s, &loginMsg);
	if (status < 0) {
		printf("Error while sending login message\n");
		return ERROR;
	}

	MSG loginStatus;

	status = getMessage(s, &loginStatus);
	if (status < 0) {
		printf("Error while getting login status\n");
		return ERROR;
	}

	if (loginStatus.opcode == LOGIN_SUCCESS) {
		printf("Connected to server\n");
		return OK;
	} else if (loginStatus.opcode == LOGIN_FAIL)
		printf("Username or password incorrect\n");
	else
		printf("Error while getting login status\n");

	return ERROR;

}

int showInbox(SOCKET s) {
	int status = OK;
	MSG cmnd;
	cmnd.opcode = SHOW_INBOX;
	cmnd.length = strlen(user.username) + 1;
	strcpy(cmnd.msg, user.username);

	status = sendMessage(s, &cmnd);
	if (status < 0) {
		printf("Sending show Inbox command failed\n");
		return ERROR;
	}
	MSG inbox;
	status = getMessage(s, &inbox);
	if (status < 0) {
		printf("Getting message command failed\n");
		return ERROR;
	}
	if (inbox.length > 0)
		printf("%s", inbox.msg);
	else {
		printf("Error while showing inbox\n");
		return ERROR;
	}

	return OK;
}

int getMail(SOCKET s, char* mail_ID) {
	int status = OK;
	MSG cmnd;
	cmnd.opcode = GET_MAIL;
	cmnd.length = strlen(user.username) + strlen(mail_ID) + 2;
	strcpy(cmnd.msg, user.username);
	strcpy(cmnd.msg + strlen(user.username) + 1, mail_ID);

	if (sendMessage(s, &cmnd) < 0) {
		printf("Sending getMail command failed\n");
		return ERROR;
	}
	MSG mailMSG;
	status = getMessage(s, &mailMSG);
	if (status < 0) {
		printf("Getting message command failed\n");
		return ERROR;
	}
	if (mailMSG.opcode == INVALID) {
		printf("Invalid mail ID.\n");
		return OK;
	}

	MAIL* mail = (MAIL*) mailMSG.msg;
	printMail(mail);
	return OK;

}

int deleteMail(SOCKET s, char* mail_ID) {
	int status = OK;
	MSG cmnd;
	cmnd.opcode = DELETE_MAIL;
	cmnd.length = strlen(user.username) + strlen(mail_ID) + 2;
	strcpy(cmnd.msg, user.username);
	strcpy(cmnd.msg + strlen(user.username) + 1, mail_ID);

	if (sendMessage(s, &cmnd) < 0) {
		printf("Sending deleteMail command failed\n");
		return ERROR;
	}
	MSG mailMSG;
	status = getMessage(s, &mailMSG);
	if (status < 0) {
		printf("Getting message command failed\n");
		return ERROR;
	}

	if (mailMSG.opcode == INVALID) {
		printf("Invalid mail ID.\n");
	}

	return OK;

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
	int status = OK;
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
		return ERROR;
	}
	/***/

	//subject
	readVarFromString(subject,MAX_SUBJECT,mail.subject,"Subject: ",9,"Error while getting subject\n")

	//text
	readVarFromString(text,MAX_CONTENT,mail.text,"Text: ",6,"Error while getting text\n")

	printMail(&mail);

	MSG mailMSG, ackMSG;
	mailMSG.length = sizeof(MAIL);
	mailMSG.opcode = COMPOSE;
	memcpy(mailMSG.msg, &mail, sizeof(MAIL));

	if (sendMessage(s, &mailMSG) < 0) {
		printf("Sending composed mail to server failed\n");
		return ERROR;
	}
	if (getMessage(s, &ackMSG) < 0) {
		printf("Getting ACK message after composing failed\n");
		return ERROR;
	}
	if(ackMSG.opcode != COMPOSE){
		printf("One or more from the recipients don't exist, composing failed\n");
		return OK;
	}
	return OK;
}

int sendQuit(SOCKET s) {
	MSG quit;
	quit.opcode = QUIT;
	quit.length = 0;
	if (sendMessage(s, &quit) < 0) {
		printf("Error while sending quit message\n");
		return ERROR;
	}
	return OK;
}

int clientProtocol(SOCKET s) {
	int status = OK;
	char input[MAX_INPUT];

	status = getWelcome(s);

	if (status < 0) {
		printf("getWelcome error\n");
		return ERROR;
	}

	status = userLogin(s);
	if (status < 0) {
		printf("Error while login\n");
		return ERROR;
	}

	while (1) {
		memset(input, 0, MAX_INPUT);
		fflush(stdout);
		fgets(input, MAX_INPUT, stdin);
		input[strlen(input) - 1] = '\0';
		if (!strcmp(input, "SHOW_INBOX")) {
			if (showInbox(s) < 0) {
				printf("Error while getting inbox\n");
				return ERROR;
			}
		} else if (!strncmp(input, "GET_MAIL", 8)) {
			if (strlen(input) < 10) {
				UnknownCommand()
			}
			if (getMail(s, input + 9)) {
				printf("Error while getting mail\n");
				return ERROR;
			}
		} else if (!strncmp(input, "COMPOSE", 8)) {
			if (composeMail(s) < 0) {
				printf("Error while composing mail\n");
				return ERROR;
			}

		} else if (!strncmp(input, "QUIT", 4)) {
			if (sendQuit(s) < 0) {
				printf("Error while sending quit\n");
				return ERROR;
			}
			return OK;
		}
	}
}

int main(int argc, char* argv[]) {

	int status = OK;

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
		return ERROR;

	}
	status = initClient(hostname, port);
	if (status < 0) {
		printf("initCLient error\n");
	}

	return status;
}
