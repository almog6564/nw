#ifndef NW_H_
#define NW_H_


#define LENGTH_SIZE sizeof(short)

#define HOST    "localhost"
#define PORT    6423
#define MAXSIZE 65536
#define WELLCOME_MSG    "Welcome! I am simple-mail-server."
#define WELLCOME_SIZE   34
#define MAX_INPUT       3000
#deinfe MAX_LOGIN       100

typedef int SOCKET;

typedef struct  {
    short  opcode;
    short  length;
    char   msg[MAXSIZE];
} MSG;

typedef enum    {
    
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










#endif