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

//Macro for reading input from the user into a string and check for general error,
//using it couple of times in the code
#define readVarFromString(string,string_size,output, prefix,prefix_size,error_msg,ret)		\
																							\
									memset(string, 0, string_size);							\
									fflush(stdout);											\
									fgets(string, string_size, stdin);						\
									string[strlen(string) - 1] = '\0';						\
																							\
									if (!strncmp(string, prefix, prefix_size)) {			\
									strcpy(output, string + prefix_size);					\
									} else {												\
									printf(error_msg);										\
									return ret;												\
									}


//Global user object - to be set in login and used afterwards until program is ended
USER user;



int clientProtocol(SOCKET);
int sendQuit(SOCKET s);

/*
 * This function gets the welcome message from the server and prints it to the screen.
 * param s	-	SOCKET of the connection
 * return ERROR if sending error or connection ended,
 otherwise return OK
 */
int getWelcome(SOCKET s) {

	//get welcome message
	MSG welcome;
	//ZERO(welcome)

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

	/** getting address info for socket creating **/
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

	/** creating socket and connecting to server **/

	SOCKET s = socket(serverinfo->ai_family, serverinfo->ai_socktype,
			serverinfo->ai_protocol);

	if (s < 0) {
		printf("Creating client socket error\n");
		return ERROR;
	}

	if (connect(s, serverinfo->ai_addr, serverinfo->ai_addrlen)) {
		printf("Connecting to server error\n");
		return ERROR;
	}

	freeaddrinfo(serverinfo);

	//call to main client flow program
	status = clientProtocol(s);

	if (close(s) < 0) {
		printf("Closing socket error;\n");
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
	memset(input, 0, MAX_LOGIN);
	fflush(stdout);
	fgets(input, MAX_LOGIN, stdin);
	input[strlen(input) - 1] = '\0';

	if (!strncmp(input, "User: ", 6)) {
		strcpy(username, input + 6);
	} else if (!strcmp(input, "QUIT")) {
		sendQuit(s);
		return QUIT;
	} else {
		printf("You are not logged in. Please log in by writing exactly:\n"
			"User: username\nPassword: password\n");
		return LOGIN_FAIL;
	}

	//password
	readVarFromString(input,MAX_LOGIN,password,"Password: ",10,
			"Error while getting password\nPlease log in by writing exactly:\n"
			"User: username\nPassword: password\n",
			LOGIN_FAIL)

	strcpy(user.username, username);
	strcpy(user.password, password);

	int uslen = strlen(username) + 1, pslen = strlen(password) + 1;

	//send user info to server
	MSG loginMsg;
	ZERO(loginMsg)
	loginMsg.opcode = LOGIN;
	loginMsg.length = uslen + pslen;

	strncpy(loginMsg.msg, username, uslen);
	strncpy(loginMsg.msg + uslen, password, pslen);

	status = sendMessage(s, &loginMsg);
	if (status < 0) {
		printf("Error while sending login message\n");
		return ERROR;
	}

	//get login status back from server
	MSG loginStatus;
	ZERO(loginStatus)
	status = getMessage(s, &loginStatus);
	if (status < 0) {
		printf("Error while getting login status\n");
		return ERROR;
	}
	if (loginStatus.opcode == LOGIN_SUCCESS) { //login successful, can continue
		printf("Connected to server\n");
		return LOGIN_SUCCESS;
	} else if (loginStatus.opcode == LOGIN_FAIL) { //opcode sent when some problem with username/password
		printf("Username or password incorrect\n");
		return LOGIN_FAIL;
	} else
		printf("Error while getting login status\n"); //undefined error

	return ERROR;

}

/*
 * This function sends the SHOW_INBOX command to the server and retrieves the inbox sent back
 * param s		-	network socket to send on
 * return ERROR if sending error or connection ended, or if message was invalid
 otherwise return OK
 */
int showInbox(SOCKET s) {
	int status = OK;

	//send SHOW_INBOX command to server
	MSG cmnd;
	ZERO(cmnd)
	cmnd.opcode = SHOW_INBOX;
	cmnd.length = 0;

	status = sendMessage(s, &cmnd);
	if (status < 0) {
		printf("Sending show Inbox command failed\n");
		return ERROR;
	}

	//get SHOW_INBOX message back
	MSG inbox;
	ZERO(inbox)
	status = getMessage(s, &inbox);
	if (status < 0) {
		printf("Getting message command failed\n");
		return ERROR;
	}
	if (inbox.opcode != SHOW_INBOX) {
		printf("Invalid message\n");
		return ERROR;
	}

	//the first message includes a number (as a string) that describes the number
	//of mails that are in the inbox, for the loop ahead. if the message length is 0,
	//it means that the inbox is empty
	if (inbox.length > 0) {

		int numOfMails = atoi(inbox.msg);

		//iterate through mails, each one sent as a new message
		for (int mail = 0; mail < numOfMails; mail++) {
			MSG cur;
			ZERO(cur)
			if (getMessage(s, &cur) < 0) {
				printf("Error while getting inbox message\n");
				return ERROR;
			}
			if (cur.opcode != SHOW_INBOX) {
				printf("Invalid message\n");
				return ERROR;
			}
			printf("%s", cur.msg);
		}
	}

	return OK;
}

/*
 * This function sends the "GET_MAIL [mail_ID]" command to the server and retrieves the mail sent back
 * The function does not parse the mail_ID string and lets the server do it.
 * param s			-	network socket to send on
 * param mail_ID	-	pointer to start of string representing the mailid command
 * return ERROR if sending error or connection ended, or if message was invalid
 otherwise return OK (even if mail_ID was invalid)
 */
int getMail(SOCKET s, char* mail_ID) {
	int status = OK;

	//send GET_MAIL command with mail_ID as text of message
	MSG cmnd;
	ZERO(cmnd)
	cmnd.opcode = GET_MAIL;
	cmnd.length = strlen(mail_ID) + 1;
	strcpy(cmnd.msg, mail_ID);

	if (sendMessage(s, &cmnd) < 0) {
		printf("Sending getMail command failed\n");
		return ERROR;
	}

	//getting message back - if opcode is INVALID it means the mail_ID either represents a N/A
	//mail or contains invalid characters
	MSG mailMSG;
	ZERO(mailMSG)
	status = getMessage(s, &mailMSG);
	if (status < 0) {
		printf("Getting message command failed\n");
		return ERROR;
	}
	if (mailMSG.opcode == INVALID) {
		printf("Invalid mail ID.\n");
		return OK;
	}

	//MAIL casting from char[]
	MAIL* mail = (MAIL*) mailMSG.msg;
	printMail(mail);
	return OK;

}

/*
 * This function sends the "DELETE_MAIL [mail_ID]" command to the server and retrieves an ack message back
 * The function does not parse the mail_ID string and lets the server do it.
 * param s			-	network socket to send on
 * param mail_ID	-	pointer to start of string representing the mailid command
 * return ERROR if sending error or connection ended, or if message was invalid
 otherwise return OK (even if mail_ID was invalid)
 */
int deleteMail(SOCKET s, char* mail_ID) {
	int status = OK;

	//send GET_MAIL command with mail_ID as text of message
	MSG cmnd;
	ZERO(cmnd)
	cmnd.opcode = DELETE_MAIL;
	cmnd.length = strlen(mail_ID) + 1;
	strcpy(cmnd.msg, mail_ID);

	if (sendMessage(s, &cmnd) < 0) {
		printf("Sending deleteMail command failed\n");
		return ERROR;
	}

	//getting message back - if opcode is INVALID it means the mail_ID either represents a N/A
	//mail or contains invalid characters
	MSG mailMSG;
	ZERO(mailMSG)
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

/*
 * This function parses a string of recipients separated by commas (with NO SPACES AT ALL)
 * and inserts them in *pmail->to[].
 * The functions assumes a non empty string. The function does not check for correctness of the string.
 * Meaning, if there are spaces they will be treated as part of the usernames, and if there are no commas
 * the function will treat the whole string as one username
 * param to			-	string of recipients
 * param pmail		-	pointer to the mail strcut
 */
void getRecipients(char to[], MAIL* pmail) {
	short toLen = 1;
	char* cpy = to;

	//change each comma to 0 for convenience, also count number of recip.
	for (; *cpy != '\0'; cpy++) {
		if (*cpy == ',') {
			toLen++;
			*cpy = 0;
		}
	}
	pmail->toLen = toLen;

	cpy = to;

	//copy each one (use strcpy because 0 separates between names)
	for (int i = 0; i < toLen; i++) {
		strcpy(pmail->to[i], cpy);
		cpy += (strlen(cpy) + 1);
	}

}

/*
 * This function gets inputs from the user to compose a new mail. The function checks for the exact format:
 * To: user1,user2,...
 * Subject: subject...
 * Text: content...
 * If the format is wrong the function prints an error message to the user, and lets him try again.
 * The function uses getRecipients to parse the "To" line.
 * The function then sends the mail to the server and waits for ack message.
 * If one or more of the recipients doesn't exists, the function prompts to the user.
 * param s			-	network socket to send on
 *
 * return ERROR if sending or getting error or connection ended otherwise return OK
 */
int composeMail(SOCKET s) {
	char to[MAX_COMPOSE_TO]; //enough for commas
	char subject[MAX_SUBJECT];
	char text[MAX_CONTENT];
	MAIL mail;
	ZERO(mail)
	strcpy(mail.from, user.username);

	/* " To: "  line */
	memset(to, 0, MAX_COMPOSE_TO);
	fflush(stdout);
	fgets(to, MAX_COMPOSE_TO, stdin);
	to[strlen(to) - 1] = '\0';

	if (strncmp(to, "To: ", 4)) {
		printf("Invalid command. Use exactly:\nCOMPOSE\n"
			"To: user1,user2,...\nSubject: subject...\nText: content...\n");
		return OK;
	}
	//if after "To: " there is nothing
	if (*(to + 4) == 0) {
		printf("Message must have recipients\n");
		return OK;
	}

	getRecipients(to + 4, &mail);

	//subject
	readVarFromString(subject,MAX_SUBJECT,mail.subject,"Subject: ",9,"Error while getting subject\n",OK)

	//text
	readVarFromString(text,MAX_CONTENT,mail.text,"Text: ",6,"Error while getting text\n",OK)

	//sending of mail
	MSG mailMSG, ackMSG;
	ZERO(mailMSG)
	ZERO(ackMSG)

	mailMSG.length = sizeof(MAIL);
	mailMSG.opcode = COMPOSE;

	//cast the mail struct to the string msg as bits
	memcpy(mailMSG.msg, &mail, sizeof(MAIL));

	if (sendMessage(s, &mailMSG) < 0) {
		printf("Sending composed mail to server failed\n");
		return ERROR;
	}

	//get ack message
	if (getMessage(s, &ackMSG) < 0) {
		printf("Getting ACK message after composing failed\n");
		return ERROR;
	}

	//if wrong then it means the server returned a message that recip. are invalid
	if (ackMSG.opcode != COMPOSE) {
		printf(
				"One or more from the recipients don't exist, composing failed\n");
	}
	return OK;
}

/*
 * This function sends the QUIT signal to the server.
 * param s		-	network socket to send on
 *
 * return ERROR if sending or getting error or connection ended otherwise return OK
 */
int sendQuit(SOCKET s) {
	MSG quit;
	ZERO(quit)

	quit.opcode = QUIT;
	quit.length = 0;
	if (sendMessage(s, &quit) < 0) {
		printf("Error while sending quit message\n");
		return ERROR;
	}
	return OK;
}

/*
 * This function sets the main flow of the client program from the moment the connection is established,
 * including getting the commands from the user and calling all the command functions
 * param s		-	network socket to send on
 *
 * return ERROR if sending or getting error or connection ended otherwise return OK
 */
int clientProtocol(SOCKET s) {
	int status = OK;
	char input[MAX_INPUT];

	//get welcome message
	status = getWelcome(s);

	if (status < 0) {
		printf("Error while getting the welcome message\n");
		return ERROR;
	}

	//User login flow
	while (1) {
		status = userLogin(s);
		if (status == LOGIN_SUCCESS)
			break;
		else if (status == LOGIN_FAIL) //login failed for some "fixable" reason. try login again
			continue;
		else if (status == QUIT) //user wrote "QUIT" before login, quit the connection
			return OK;
		else
			return ERROR; //error in connection inside userLogin
	}

	//Main commands flow
	while (1) {

		//get command from user
		memset(input, 0, MAX_INPUT);
		fflush(stdout);
		fgets(input, MAX_INPUT, stdin);
		input[strlen(input) - 1] = '\0';

		//Check the command
		if (!strcmp(input, "SHOW_INBOX")) {
			if (showInbox(s) < 0) {
				printf("Error while getting inbox\n");
				return ERROR;
			}
		} else if (!strncmp(input, "GET_MAIL", 8)) {
			if (strlen(input) < 10) { //no chars after GET_MAIL
				UnknownCommand()
			}
			if (getMail(s, input + 9)) {
				printf("Error while getting mail\n");
				return ERROR;
			}
		} else if (!strncmp(input, "DELETE_MAIL", 11)) {
			if (strlen(input) < 13) { //no chars after DELETE_MAIL
				UnknownCommand()
			}
			if (deleteMail(s, input + 12) < 0) {
				printf("Error while deleting mail\n");
				return ERROR;
			}
		} else if (!strncmp(input, "COMPOSE", 8)) {
			if (composeMail(s) < 0) {
				printf("Error while composing mail\n");
				return ERROR;
			}

		} else if (!strncmp(input, "QUIT", 4)) {
			if (sendQuit(s) < 0) {
				return ERROR;
			}
			return OK;
		} else {
			UnknownCommand()
		}
	}
}

int main(int argc, char* argv[]) {

	char hostname[MAX_LEN];
	int port = DEFAULT_PORT;

	switch (argc) {
	case 1: //no arguments provieded - continuning with defaults
		strcpy(hostname, HOST);
		break;

	case 3: //both arguments
		port = atoi(argv[2]);
		if (!port) { //invalid value (not a number or zero which is invalid port for us)
			printf("Invalid arguments.\nUse: mail_client [hostname [port]]\n");
			return ERROR;
		}

	case 2: //only hostname provided, default port is to be used
		strcpy(hostname, argv[1]);
		break;

	default:
		printf("Invalid arguments.\nUse: mail_client [hostname [port]]\n");
		return ERROR;

	}

	return initClient(hostname, port);
}
