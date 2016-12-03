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
#include <stdbool.h>
#include "message.h"

USERLIST lst;
ACTIVEUSER gUser;

int createUsersList(char* path) {
	memset(&lst, 0, sizeof(lst));
	char line[LINE_LEN];
	int i = 0, chk;
	FILE* fp = fopen(path, "r");
	if (!fp) {
		printf("Error while opening users file");
		return ERROR;
	}
	while (fgets(line, sizeof(line), fp) != NULL && i < MAX_USERS) {
		chk = sscanf(line, "%s %s", lst.list[i].username, lst.list[i].password);
		if (chk < 2 || chk == EOF) {
			printf("Error while reading users file, invalid format\n");
			return ERROR;
		}
		i++;
		lst.size++;
	}
	fclose(fp);
	return OK;
}

int login(SOCKET s) {
	int i;
	MSG logMsg, connected;
	USER usr;
	char username[MAX_LEN], password[MAX_LEN];
	int status = getMessage(s, &logMsg);
	if (status < 0) {
		return ERROR; // ERROR
	} else if (logMsg.opcode == QUIT)
		return QUIT;

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
			if (sendMessage(s, &inv) < 0) {
				printf("Error sending invalid message\n");
				return ERROR;
			}
			return LOGIN_FAIL;
		}
	}

	gUser.userID = i;
	memcpy(&gUser.user, &lst.list[i], sizeof(USER));

	connected.opcode = LOGIN_SUCCESS;
	connected.length = 0;
	if (sendMessage(s, &connected) < 0) {
		printf("sendMessage error\n");
		return ERROR;
	}
	return LOGIN_SUCCESS;

}

int isValidMail(MAIL* mail) {
	if (strlen(mail->from) && strlen(mail->subject) && strlen(mail->text)
			&& mail->toLen > 0) {
		for (int i = 0; i < mail->toLen; i++) {
			if (!strlen(mail->to[i])) {
				return OK;
			}
		}
		return 1;
	}
	return OK;

}

int showInbox(SOCKET s) {
	int mailid;

	if (!lst.inboxSizes[gUser.userID]) {
		MSG inbox;
		inbox.opcode = SHOW_INBOX;
		inbox.length = 0;
		if (sendMessage(s, &inbox) < 0) {
			printf("Sending inbox to client failed\n");
			return ERROR;
		}
	} else {
		int mailCntr = 0;
		for (mailid = 0; mailid < lst.inboxSizes[gUser.userID]; mailid++) {
			if (lst.isMail[gUser.userID][mailid])
				mailCntr++;
		}
		MSG num;
		num.opcode = SHOW_INBOX;
		sprintf(num.msg, "%d", mailCntr);
		num.length = strlen(num.msg) + 1;
		if (sendMessage(s, &num) < 0) {
			printf("Sending inbox to client failed\n");
			return ERROR;
		}

		for (mailid = 0; mailid <= lst.inboxSizes[gUser.userID]; mailid++) {
			if (lst.isMail[gUser.userID][mailid]) {
				MSG mailMSG;
				mailMSG.opcode = SHOW_INBOX;
				sprintf(mailMSG.msg, "%d %s \"%s\"\n", mailid + 1,
						lst.inbox[gUser.userID][mailid].from,
						lst.inbox[gUser.userID][mailid].subject);
				mailMSG.length = strlen(mailMSG.msg) + 1;

				if (sendMessage(s, &mailMSG) < 0) {
					printf("Sending inbox to client failed\n");
					return ERROR;
				}
			}
		}

	}
	return OK;
}

int getMail(SOCKET s, MSG* _getMailMsg) {
	int mid;
	MSG getMail = *_getMailMsg, mailMSG;
	char mail_id[MAX_LEN];

	strcpy(mail_id, getMail.msg);
	mid = atoi(mail_id) - 1; // mail_id-s start counting from 1 but are saved in array from 0

	if (mid < 0 || !lst.inboxSizes[gUser.userID]) {
		mailMSG.length = 0;
		mailMSG.opcode = INVALID;
	} else {
		mailMSG.length = sizeof(MAIL);
		mailMSG.opcode = GET_MAIL;
		memcpy(mailMSG.msg, (char*) (&lst.inbox[gUser.userID][mid]),
				sizeof(MAIL));
	}
	if (sendMessage(s, &mailMSG) < 0) {
		printf("Sending the mail to server failed\n");
		return ERROR;
	}
	return OK;
}

int receiveMail(SOCKET s, MSG* _mailMsg) {
	int i = 0, j = 0;
	MAIL mail;
	MSG mailSent, opInval;
	MSG mailMsg = *_mailMsg;

	memcpy(&mail, (MAIL*) mailMsg.msg, sizeof(MAIL));
	//extract the parameters of the mail from the msg
	for (; i < mail.toLen; i++) {
		for (; j < lst.size; j++) {
			if (!strcmp(lst.list[j].username, mail.to[i])) {
				memcpy(&(lst.inbox[j][lst.inboxSizes[j]]), &mail, sizeof(MAIL));
				lst.isMail[j][lst.inboxSizes[j]] = 1; //mail exists
				lst.inboxSizes[j]++;
				break;

			} else if (j == lst.size - 1) {
				opInval.opcode = INVALID;
				opInval.length = 0;
				sendMessage(s, &opInval);
				break;
			}
		}
		j = 0;

	}

	mailSent.opcode = COMPOSE;
	mailSent.length = 0;
	if (sendMessage(s, &mailSent) < 0) {
		printf("sendMessage for mailSent error\n");
		return ERROR;
	}
	return OK;
}

int deleteMail(SOCKET s, MSG* delmsg) {
	int mailID = atoi(delmsg->msg) - 1;
	if (mailID < 0 || mailID > MAXMAILS) {
		MSG inv;
		inv.opcode = INVALID;
		inv.length = 0;
		if (sendMessage(s, &inv) < 0) {
			printf("Error while sending invalid message in deleteMail\n");
			return ERROR;
		}
	}

	//Deleting:
	//If mail doesnt exist it doesnt matter
	//memset(&lst.inbox[gUser.userID][mailID], 0, sizeof(MAIL))
	lst.isMail[gUser.userID][mailID] = 0;
	if (mailID + 1 == lst.inboxSizes[gUser.userID])
		lst.inboxSizes[gUser.userID]--;
	MSG ok;
	ok.opcode = DELETE_MAIL;
	ok.length = 0;
	if (sendMessage(s, &ok) < 0) {
		printf("Error while sending ack message in deleteMail\n");
		return ERROR;
	}
	return OK;

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
		return ERROR;
	}

	while (1) {
		int status = login(s);

		if (status == LOGIN_SUCCESS) {
			break;
		} else if (status == LOGIN_FAIL) {
			continue;
		} else if (status == QUIT) {
			memset(&gUser, 0, sizeof(ACTIVEUSER));
			return QUIT;
		} else
			return ERROR;
	}

	while (1) { //Main commands loop. suppose to continue until getting QUIT opcode
		MSG get;
		if (getMessage(s, &get) < 0) {
			printf("Error while getting message on server\n");
			return ERROR;
		}
		switch (get.opcode) {

		case QUIT:
			memset(&gUser, 0, sizeof(ACTIVEUSER));
			return QUIT;

		case SHOW_INBOX:
			if (showInbox(s) < 0) {
				printf("Error while showing inbox\n");
				return ERROR;
			}
			break;

		case GET_MAIL:
			if (getMail(s, &get) < 0) {
				printf("Error while getting mail\n");
				return ERROR;
			}
			break;

		case COMPOSE:
			if (receiveMail(s, &get) < 0) {
				printf("Error while getting composed message\n");
				return ERROR;
			}
			break;

		case DELETE_MAIL:
			if (deleteMail(s, &get) < 0) {
				printf("Error while deleting mail\n");
				return ERROR;
			}
			break;
		}
	}

	return OK;
}

int initServer(int port) {

	SOCKET sock = socket(PF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		printf("socket error\n");
		return ERROR;
	}

	struct sockaddr_in my_addr, client_addr;

	struct in_addr _addr;
	_addr.s_addr = htonl(INADDR_ANY);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr = _addr;

	if (bind(sock, (struct sockaddr *) (&my_addr), sizeof(my_addr)) < 0) {
		printf("bind error\n");
		return ERROR;
	}

	if (listen(sock, 10) < 0) {
		printf("listen error\n");
		return ERROR;
	}

	socklen_t sin_size = sizeof(struct sockaddr_in);

	//maybe need to be in a loop:

	while (1) { //Main client connecting loop. after a quit waits for another accept
		SOCKET s = accept(sock, (struct sockaddr *) (&client_addr), &sin_size);
		if (s < 0) {
			printf("Client accept error\n");
			return ERROR;
		}

		int status = serverProcess(s);
		if (close(s) < 0) {
			printf("Closing socket failed\n");
			return ERROR;
		}
		if (status < 0) {
			return ERROR;
		} else if (status == QUIT || status == LOGIN_FAIL) {
			continue;
		} else
			break;

	}

	return OK;

}

int main(int argc, char* argv[]) {

	memset(&lst, 0, sizeof(lst));
	memset(&gUser, 0, sizeof(ACTIVEUSER));

	//check arguments
	int port = DEFAULT_PORT;
	switch (argc) {

	case 3:
		port = atoi(argv[2]);

	case 2:
		if (createUsersList(argv[1]) < 0) {
			return ERROR;
		}

		break;

	default:
		printf("Invalid arguments.\nTry: mail_server users_file [port]\n");
		return ERROR;

	}

	SOCKET s = initServer(port);
	if (s < 0) {
		printf("Server Error\n");
		return ERROR;
	}

	return OK;

}
