#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "nw.h"

/*
 * This function gets a mail and prints its sender, recipients,
 * subject and content
 * param m	-	pointer to a mail
 * return OK
 */

void printMail(MAIL* m) {
	if (!m)
		return;

	printf("From: %s\n", m->from);
	printf("To: ");
	for (int i = 0; i < m->toLen; i++) {
		printf("%s", m->to[i]);
		if (i < m->toLen - 1) {
			printf(",");
		}
	}
	printf("\n");
	printf("Subject: %s\n", m->subject);
	printf("Text: %s\n", m->text);
}

/*
 * This function gets a buffer and reads from it,
 * param s	-	SOCKET of the connection
 * param _buff - pointer to a buffer
 * param len - size of buffer
 * return ERROR if connection ends,
 otherwise return OK
 */

int getBuffer(SOCKET s, void* buff, int len) {

	int recvSize;
	int tot = 0;
	int leftToRead = len;

	while (tot < leftToRead) {
		recvSize = recv(s, (void*) ((long) buff + tot), leftToRead, 0);
		switch (recvSize) {
		case -1:
			printf("Receiving buffer failed");
			return ERROR;

		case 0:
			printf("Connection ended while receiving message\n");
			return ERROR;

		default:
			tot += recvSize;
			leftToRead -= recvSize;
			break;

		}
	}
	return tot;
}

/*
 * This function gets a message and reads its content
 * param s	-	SOCKET of the connection
 * param message - pointer to message from the client
 * return ERROR if receiving the buffer failed or message is too long,
 otherwise return OK
 */

int getMessage(SOCKET s, MSG* message) {
	if (getBuffer(s, &message->opcode, LENGTH_SIZE) < 0)
		return ERROR;
	message->opcode = ntohs(message->opcode);
	if (getBuffer(s, &message->length, LENGTH_SIZE) < 0)
		return ERROR;
	message->length = ntohs(message->length); //so it would be BIG endian

	if (message->length > MAXSIZE) {
		printf("Message got is too long, failing..\n");
	}

	return getBuffer(s, message->msg, message->length);
}

/*
 * This function gets a message and sends it,
 * param s	-	SOCKET of the connection
 * param message - pointer to message for the client
 * return ERROR if sending message failed,
 otherwise return OK
 */

int sendMessage(SOCKET s, MSG* message) {

	int sendSize = 2 * LENGTH_SIZE + message->length;
	int leftToSend = sendSize;

	int tot = 0;
	int sent;

	MSG copy;
	copy.opcode = htons(message->opcode);
	copy.length = htons(message->length);
	memcpy(copy.msg, message->msg, message->length);


	while (tot < sendSize) {

		sent = send(s, &copy + tot, leftToSend, 0);

		switch (sent) {
		case -1:
			printf("Sending message failure\n");
			return ERROR;

		case 0:
			printf("Connection ended while sending message\n");
			return OK;

		default:
			tot += sent;
			leftToSend -= sent;
			break;

		}
	}
	return tot;

}

