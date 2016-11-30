#ifndef NW_H_
#define NW_H_

#define LENGTH_SIZE     sizeof(short)
#define HOST            "localhost"
#define PORT            6423
#define MAXSIZE         65536
#define WELLCOME_MSG    "Welcome! I am simple-mail-server."
#define WELLCOME_SIZE   34
#define MAX_INPUT       3000
#define MAX_LOGIN       100
#define MAX_LEN         50
#define MAX_USERS       20
#define LINE_LEN        1000
#define TOTAL_TO        20
#define MAX_SUBJECT     100
#define MAX_CONTENT     2000
#define MAXMAILS        32000

typedef int SOCKET;

typedef struct {
	short opcode;
	short length;
	char msg[MAXSIZE];
} MSG;

typedef enum {

	INVALID = -1,
	WELLCOME = 0,
	LOGIN,
	LOGIN_SUCCESS,
	LOGIN_FAIL,
	SHOW_INBOX,
	GET_MAIL,
	DELETE_MAIL,
	COMPOSE,
	QUIT

} OPCODE;

struct MAIL;

typedef struct {
	char username[MAX_LEN];
	char password[MAX_LEN];
	MAIL* inbox;
	int inboxUsed;
	int inboxSize;
} USER;

#pragma pack(push,1)
typedef struct {
	char from[MAX_LEN];
	char to[TOTAL_TO];
	short toLen;
	char subject[MAX_SUBJECT];
	char text[MAX_CONTENT];
} MAIL;
#pragma pack(pop)

typedef struct {
	USER list[MAX_USERS];
	short size;
} USERLIST;

#define UnknownCommand()    printf("Unknown Command\n");    return -1;

#endif
